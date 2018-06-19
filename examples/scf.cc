#include "scf.h"

#include <fmt/format.h>

#include <cstdlib>

using namespace ::vt;

static NodeType this_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct TestMsg : ::vt::Message {};

static void testHandler(TestMsg* msg) {
  ::fmt::print("{}: testHandler\n", theContext()->getNode());
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  this_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();
  libint2::initialize();

  ::fmt::print("{}: started\n", this_node);

  if (this_node == 0) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->broadcastMsg<TestMsg, testHandler>(msg);
  }

  std::vector<scf::Water> waters(1);
  waters[0] = scf::Water(
    {0, -0.07579, 0}, {0.86681, 0.60144, 0.0}, {-0.86681, 0.60144, 0.0});

  for (auto const& sh : waters[0].shells()) {
    std::cout << sh << std::endl;
  }

  const auto num_waters = waters.size();

  // All the matrices we will need, trim down later.
  scf::SparseMatrix S(num_waters, num_waters);
  scf::SparseMatrix H(num_waters, num_waters);
  scf::SparseMatrix F(num_waters, num_waters);


  // Get engine for V_integrals
  {
    const auto max_nprim = 6; // Water S core has 6 primitives
    const auto max_ang = 1;   // Water 6-31g max ang_mo is p -> 1
    libint2::Engine eng_o(libint2::Operator::overlap, max_nprim, max_ang, 0);
    libint2::Engine eng_t(libint2::Operator::kinetic, max_nprim, max_ang, 0);
    libint2::Engine eng_v(libint2::Operator::nuclear, max_nprim, max_ang, 0);

    eng_v.set_params(nuclei_charges(waters));

    // Populate the matrices with information
    for (auto i = 0; i < num_waters; ++i) {
      auto const& wi = waters[i];
      for (auto j = 0; j < num_waters; ++j) {
        if (S.is_local(i, j)) { // Assume that H has the same distribution as S
          auto const& wj = waters[j];
          S.block(i, j) = scf::one_body_integral(wi, wj, eng_o);
          H.block(i, j) = scf::one_body_integral(wi, wj, eng_v) +
            scf::one_body_integral(wi, wj, eng_t);
        }
      }
    }
  }

  auto truncate_thresh = 1e-12;
  S.truncate(truncate_thresh);
  H.truncate(truncate_thresh);

  // Initialize guess assuming that D = 0;
  F = H;

  // Bootstrap SCF
  auto Srep = S.replicate();
  auto Frep = F.replicate();
  auto Hrep = H.replicate();
  std::cout << "S:\n" << Srep << "\n" << std::endl;
  std::cout << "H:\n" << Hrep << "\n" << std::endl;


  auto nocc = 0;
  for (auto const& w : waters) {
    nocc += w.nelectrons();
  }
  nocc /= 2;

  auto make_new_density = [num_waters, nocc](auto F, auto S, double thresh) {
    // TODO Compute on one node and bcast, luckily Eigen will work this way
    // since there are no phase issues
    Eigen::GeneralizedSelfAdjointEigenSolver<scf::MatrixBlock> gevd(F, S);
    scf::MatrixBlock const& C = gevd.eigenvectors().leftCols(nocc);
    scf::MatrixBlock Drep = C * C.transpose();

    return scf::SparseMatrix(num_waters, num_waters, Drep, thresh);
  };

  auto D = make_new_density(Frep, Srep, truncate_thresh);
  auto Drep = D.replicate();


  libint2::Engine eng_2e(libint2::Operator::coulomb, 6, 1, 0);
  scf::MatrixBlock Q;
  std::vector<std::vector<scf::IndexType>> sparse_pair_list;
  std::tie(Q, sparse_pair_list) = scf::Schwarz(waters, eng_2e);

  std::cout << "Input D:\n" << Drep << "\n" << std::endl;

  auto enuc = 0.0;
  {

    std::vector<scf::Atom> atoms;
    atoms.reserve(num_waters * 3);
    for (auto const& w : waters) {
      for (auto const& a : w.atoms()) {
        atoms.push_back(a);
      }
    }

    for (auto i = 0; i < atoms.size(); i++) {
      for (auto j = i + 1; j < atoms.size(); j++) {
        double dist = (atoms[i].r - atoms[j].r).norm();
        enuc += atoms[i].atomic_number * atoms[j].atomic_number / dist;
      }
    }
  }
  ::fmt::print("Nuclear repulsion energy: {}", enuc);

  auto iter = 0;
  while (30 != iter++) {
    ::fmt::print("Starting iteration {}\n", iter);

    // TODO Make a new fock matrix given the density
    scf::SparseMatrix Fnew(num_waters, num_waters);
    const auto schwarz_thresh = truncate_thresh;
    four_center_update(
      Fnew, D, Q, sparse_pair_list, waters, schwarz_thresh, eng_2e);

    F.truncate(truncate_thresh);
    F = std::move(Fnew);
    Frep = F.replicate();
    if (iter < 2) {
      std::cout << "Iter(" << iter << ") F:\n" << Frep << "\n" << std::endl;
    }
    std::cout << "Iter(" << iter
              << ") F symm error: " << (Frep - Frep.transpose()).norm()
              << std::endl;
    Frep += Hrep;

    D = make_new_density(Frep, Srep, truncate_thresh);
    Drep = D.replicate();

    auto energy = enuc;
    for (auto i = 0; i < Drep.rows(); ++i) {
      for (auto j = 0; j < Drep.cols(); ++j) {
        energy += Drep(i, j) * (Frep(i, j) + Hrep(i, j));
      }
    }
    ::fmt::print("\tenergy: {}\n", energy);
  }


  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();
  libint2::finalize(); // done with libint

  return 0;
}

std::pair<scf::MatrixBlock, std::vector<std::vector<scf::IndexType>>>
scf::Schwarz(std::vector<Water> const& waters, libint2::Engine& eng) {
  const auto num_waters = waters.size();
  // To start off let's just do screening on individual waters, we can make it
  // do shell level screening later.
  scf::MatrixBlock Q(num_waters, num_waters);
  Q.setZero();

  std::vector<std::vector<scf::IndexType>> sparse_pair_list(num_waters);

  // TODO should distribute and reduce the matrix and the pair list
  for (auto i = 0; i < num_waters; ++i) {
    auto const& wi = waters[i];

    for (auto j = 0; j < num_waters; ++j) {
      auto const& wj = waters[j];

      Q(i, j) = scf::two_e_integral(wi, wj, wi, wj, eng).norm();

      if (Q(i, j) >= 1e-12) { // Screen the sparse pairs
        sparse_pair_list[i].push_back(j);
      }
    }
  }

  return std::make_pair(std::move(Q), std::move(sparse_pair_list));
}

void scf::four_center_update(
  scf::SparseMatrix& F, scf::SparseMatrix const& D, MatrixBlock const& Q,
  std::vector<std::vector<scf::IndexType>> const& sparse_list,
  std::vector<scf::Water> const& waters, double screen, libint2::Engine& eng) {

  eng.set_precision(0.0);

  auto do_J = [screen](double Qij, double Qkl, double Dkl_norm) {
    return Qij * Qkl * Dkl_norm >= screen;
  };

  auto do_K = [screen](double Qil, double Qkj, double Dkj_norm) {
    return Qil * Qkj * Dkj_norm >= screen;
  };

  // TODO come back and add exchange
  const auto nwaters = waters.size();
  const auto nblock = scf::nbasis_per_water;
  for (auto i = 0; i < nwaters; ++i) {
    // for (auto j : sparse_list[i]) {
    for (auto j = 0; j < nwaters; ++j) {
      const auto Qij = Q(i, j); // Coulomb Bra
      auto& Fij = F.block(i, j);

      for (auto k = 0; k < nwaters; ++k) {
        const auto Qkj = Q(k, j); // Exchange ket

        auto& Dkj = D.block(k, j); // Exchange D
        const auto Dkj_norm = Dkj.norm();

        // for (auto l : sparse_list[k]) {
        for (auto l = 0; l < nwaters; ++l) {
          const auto Qkl = Q(k, l); // Coulomb ket
          const auto Qil = Q(i, l); // Exchange bra

          auto& Dkl = D.block(k, l); // Coulomb D

          // if (do_J(Qij, Qkl, Dkl.norm()) || do_K(Qil, Qkj, Dkj_norm)) {
          if (Dkl.norm() + Dkj.norm() > 0) {
            auto& Fil = F.block(i, l);

            // We may want to dive down into this and do shell level screening
            // auto ij_kl_ints = scf::two_e_integral(
            //   waters[i], waters[j], waters[k], waters[l], eng);

            if (Fij.size() == 0) {
              Fij.resize(scf::nbasis_per_water, scf::nbasis_per_water);
              Fij.setZero();
            }

            if (Fil.size() == 0) {
              Fil.resize(scf::nbasis_per_water, scf::nbasis_per_water);
              Fil.setZero();
            }

            auto const& buf = eng.results();

            auto s1_start = 0;
            for (auto const& s1 : waters[i].shells()) {
              const auto s1_size = s1.size();

              auto s2_start = 0;
              for (auto const& s2 : waters[j].shells()) {
                const auto s2_size = s2.size();

                auto s3_start = 0;
                for (auto const& s3 : waters[k].shells()) {
                  const auto s3_size = s3.size();

                  auto s4_start = 0;
                  for (auto const& s4 : waters[l].shells()) {
                    const auto s4_size = s4.size();

                    eng.compute(s1, s2, s3, s4);
                    auto const& buf_1234 = buf[0];

                    // write the matrix
                    for (auto p = 0, pqrs = 0; p < s1_size; ++p) {
                      const auto bf1 = p + s1_start;

                      for (auto q = 0; q < s2_size; ++q) {
                        const auto bf2 = q + s2_start;

                        for (auto r = 0; r < s3_size; ++r) {
                          const auto bf3 = r + s3_start;

                          for (auto s = 0; s < s4_size; ++s, ++pqrs) {
                            const auto bf4 = s + s4_start;

                            const auto value = buf_1234[pqrs];
                            Fij(bf1, bf2) += 2 * Dkl(bf3, bf4) * value;
                            Fil(bf1, bf4) -=  Dkj(bf3, bf2) * value;
                          }
                        }
                      }
                    }

                    s4_start += s4_size;
                  }
                  s3_start += s3_size;
                }
                s2_start += s2_size;
              }
              s1_start += s1_size;
            }
          }
        }
      }
    }
  }
}

std::vector<std::pair<double, std::array<double, 3>>>
scf::nuclei_charges(std::vector<scf::Water> const& waters) {
  std::vector<std::pair<double, std::array<double, 3>>> q;

  for (auto const& w : waters) {
    for (auto const& a : w.atoms()) {
      double an = a.atomic_number;
      q.push_back({an, {a.r[0], a.r[1], a.r[2]}});
    }
  }

  return q;
}

// Until we have integral kernels just fill with dummy data
scf::MatrixBlock scf::two_e_integral(
  Water const& i, Water const& j, Water const& k, Water const& l,
  libint2::Engine& eng) {

  const auto nbasis = i.nbasis();
  scf::MatrixBlock out(nbasis * nbasis, nbasis * nbasis);
  out.setZero();

  auto const& buf = eng.results();

  auto s1_start = 0;
  for (auto const& s1 : i.shells()) {
    const auto s1_size = s1.size();

    auto s2_start = 0;
    for (auto const& s2 : j.shells()) {
      const auto s2_size = s2.size();

      auto s3_start = 0;
      for (auto const& s3 : k.shells()) {
        const auto s3_size = s3.size();

        auto s4_start = 0;
        for (auto const& s4 : l.shells()) {
          const auto s4_size = s4.size();

          eng.compute(s1, s2, s3, s4);
          auto const& buf_1234 = buf[0];

          // write the matrix
          for (auto p = 0, pqrs = 0; p < s1_size; ++p) {
            for (auto q = 0; q < s2_size; ++q) {
              auto pq = (p + s1_start) * nbasis + (q + s2_start);

              for (auto r = 0; r < s3_size; ++r) {
                for (auto s = 0; s < s4_size; ++s, ++pqrs) {
                  auto rs = (r + s3_start) * nbasis + (s + s4_start);

                  out(pq, rs) = buf_1234[pqrs];
                }
              }
            }
          }

          s4_start += s4_size;
        }
        s3_start += s3_size;
      }
      s2_start += s2_size;
    }
    s1_start += s1_size;
  }

  return out;
}

scf::MatrixBlock
scf::one_body_integral(Water const& i, Water const& j, libint2::Engine& eng) {
  using libint2::Engine;
  using libint2::Operator;

  scf::MatrixBlock output(i.nbasis(), j.nbasis());
  output.setZero();

  auto const& buf = eng.results();

  auto s1_start = 0;
  for (auto const& s1 : i.shells()) {
    const auto s1_size = s1.size();
    const auto s1_end = s1_start + s1_size;
    auto s2_start = 0;

    for (auto const& s2 : i.shells()) {
      const auto s2_size = s2.size();
      const auto s2_end = s2_start + s2_size;
      eng.compute(s1, s2);

      Eigen::Map<const MatrixBlock> buf_map(buf[0], s1_size, s2_size);
      output.block(s1_start, s2_start, s1_size, s2_size) = buf_map;
      s2_start = s2_end;
    }

    s1_start = s1_end;
  }

  return output;
}

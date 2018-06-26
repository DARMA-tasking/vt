#include "scf.h"

#include <fmt/format.h>

#include <cstdlib>
#include <libint2/diis.h>

using namespace ::vt;

static NodeType this_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct TestMsg : ::vt::Message {};

static void testHandler(TestMsg* msg) {
  fmt::print("{}: testHandler\n", theContext()->getNode());
}

template <typename... Args>
void print(Args... args) {
  if (this_node == 0) {
    fmt::print(std::forward<Args>(args)...);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  this_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();
  libint2::initialize();

  print("{}: started\n", this_node);

  if (this_node == 0) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->broadcastMsg<TestMsg, testHandler>(msg);
  }

  std::vector<scf::Water> waters;
  auto num_waters = 10;
  for (auto i = 0; i < num_waters; ++i) {
    double scale = i * 10.0;
    waters.emplace_back(scf::Water(
      {0.0, -0.07579, scale}, {0.86681, 0.60144, scale},
      {-0.86681, 0.60144, scale}));
  }

  // All the matrices we will need.
  scf::SparseMatrix S(num_waters, num_waters);
  scf::SparseMatrix H(num_waters, num_waters);

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
        if (S.is_local(i, j)) {
          auto const& wj = waters[j];
          auto out_S = one_body_integral(wi, wj, eng_o);
          auto out_T = one_body_integral(wi, wj, eng_t);
          auto out_V = one_body_integral(wi, wj, eng_v);
          scf::MatrixBlock out_H = out_T + out_V;

          S.block(i, j) = std::move(out_S);
          H.block(i, j) = std::move(out_H);
        }
      }
    }
  }

  auto truncate_thresh = 1e-8;
  auto Dtruncate_thresh = 1e-6;
  S.truncate(truncate_thresh);
  H.truncate(truncate_thresh);

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
  print("Nuclear repulsion energy: {}\n", enuc);

  // Density guess is super position of water densities
  scf::SparseMatrix D(num_waters, num_waters);
  for (auto i = 0; i < num_waters; ++i) {
    if (D.is_local(i, i)) {
      D.block(i, i) = matrixBasedScf(waters[i]);
    }
  }
  print("SAWD D guess fill: {:.3f}\n", D.fillPercent());
  D.replicate();
  auto Drep = D.toEig();

  S.replicate();
  auto Srep = S.toEig();

  H.replicate();
  auto Hrep = H.toEig();

  auto nocc = 0;
  for (auto const& w : waters) {
    nocc += w.nelectrons();
  }
  nocc /= 2;

  auto make_new_density =
    [num_waters, nocc](auto F, auto S, double thresh, bool replicate = true) {
      // TODO Compute on one node and bcast, luckily Eigen will work this way
      // since there are no phase issues
      Eigen::GeneralizedSelfAdjointEigenSolver<scf::MatrixBlock> gevd(F, S);
      scf::MatrixBlock const& C = gevd.eigenvectors().leftCols(nocc);
      scf::MatrixBlock Drep = C * C.transpose();

      return scf::SparseMatrix(num_waters, num_waters, Drep, thresh, replicate);
    };

  std::vector<libint2::Engine> engines(4);
  {
    libint2::Engine eng_2e(libint2::Operator::coulomb, 6, 1, 0);
    eng_2e.set_precision(0.0); // Needs to be super tight for Q matrix
    for (auto& eng : engines) {
      eng = eng_2e;
    }
  }

  scf::MatrixBlock Q;
  std::vector<std::vector<scf::IndexType>> sparse_pair_list;
  std::tie(Q, sparse_pair_list) = scf::Schwarz(waters, engines[0]);

  //
  // Bootstrap first iteration so that we can control how close we start to
  // converged
  //

  // Make a Fock matrix using our guess density
  double c1 = 0.99;
  print("Bootstrapping SCF with {:.2f}\% SAWD guess\n", 100 * c1);
  const auto bootstrap_thresh = Dtruncate_thresh / 100.0;
  for (auto& eng : engines) {
    eng.set_precision(1e-7); // Not so tight for bootstrap
  }
  scf::SparseMatrix G(num_waters, num_waters, true);
  four_center_update(
    G, D, Q, sparse_pair_list, waters, bootstrap_thresh, engines);

  G.accumulate();

  // Control how much we actually want F to come from the SAWD guess.
  // F = c1 * Fguess + c2 * Hcore, c1 + c2 = 1
  G.replicate();
  scf::MatrixBlock Frep = c1 * (Hrep + G.toEig()) + (1.0 - c1) * Hrep;

  // Make iteration 1 density based on our modified Fock guess
  D = make_new_density(Frep, Srep, Dtruncate_thresh);
  print("Bootstrapped D fill: {:.3f}\n", D.fillPercent());

  //
  // Start SCF iterations
  //

  // Maybe we don't actually want to use diis, diis will speed up our
  // convergence, but depending on what aspect of VT we are trying to test, we
  // may want the slower convergence of not using diis.
  libint2::DIIS<scf::MatrixBlock> diis(2); // start DIIS on second iteration
  auto iter = 0;
  auto energy = Drep.cwiseProduct(Hrep + Frep).sum();
  bool converged = false;
  auto diff = 1.0;
  auto grad_error = 1.0;
  auto screen_thresh = 1e-6;
  while (!converged) {
    auto const start_time = ::vt::timing::Timing::getCurrentTime();
    print("Starting iteration {:2d}\n", iter + 1);

    screen_thresh =
      std::min(screen_thresh, std::min(1e-6, grad_error / std::abs(energy)));

    screen_thresh = std::max(1e-10, screen_thresh); // Don't get too tight

    // Steal and slightly modify libint's hf++ precision
    const auto eng_precision =
      std::min(
        screen_thresh / Q.maxCoeff(), std::numeric_limits<double>::epsilon()) /
      1296.0;

    print(
      "\tPrecisions, Fock: {:.2e}, Eng: {:.2e}, Dtrun: {:.2e}\n", screen_thresh,
      eng_precision, Dtruncate_thresh);
    for (auto& eng : engines) {
      eng.set_precision(eng_precision);
    }

    // G = H;
    G = scf::SparseMatrix(num_waters, num_waters, true);
    four_center_update(
      G, D, Q, sparse_pair_list, waters, screen_thresh, engines);
    G.accumulate();

    Frep = Hrep + G.toEig();
    Drep = D.toEig(); // D is all ready replicated
    auto old_energy = enuc + Drep.cwiseProduct(Hrep + Frep).sum();
    diff = std::abs(old_energy - energy);
    energy = old_energy;

    scf::MatrixBlock grad = Frep * Drep * Srep - Srep * Drep * Frep;
    grad_error = grad.norm() / static_cast<double>(grad.rows());
    diis.extrapolate(Frep, grad);

    D = make_new_density(Frep, Srep, Dtruncate_thresh);
    auto dfill = D.fillPercent();
    if (iter < 4) { // minimum 3 iters
      converged = false;
    } else if (iter == 99) {
      converged = true;
    } else if (diff < 1e-12 && grad_error < screen_thresh) {
      converged = true;
    } else {
      converged = false;
    }
    auto const end_time = ::vt::timing::Timing::getCurrentTime();
    print(
      "\tenergy: {0:.10f}, ediff: {1:0.3e}, grad_error: {3:0.3e}, dfill: "
      "{2:.3f}, time: {4:.6f}\n",
      energy, diff, dfill, grad_error, end_time - start_time);
    ++iter;
  }
  MPI_Barrier(MPI_COMM_WORLD); // Don't end before we print 


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
  std::vector<scf::Water> const& waters, double screen,
  std::vector<libint2::Engine>& engines) {

  auto do_J = [screen](double Qij, double Qkl, double Dkl_norm) {
    return Qij * Qkl * Dkl_norm >= screen;
  };

  auto do_K = [screen](double Qil, double Qkj, double Dkj_norm) {
    return Qil * Qkj * Dkj_norm >= screen;
  };

  // TODO come back and add exchange
  const auto nwaters = waters.size();
  const auto nblock = scf::nbasis_per_water;
  auto iter = 0ul;
  for (auto i = 0; i < nwaters; ++i) {
    for (auto j : sparse_list[i]) {
      // for (auto j = 0; j < nwaters; ++j) {
      const auto Qij = Q(i, j); // Coulomb Bra
      auto Fij = &F.block(i, j);

      for (auto k = 0; k < nwaters; ++k, ++iter) {
        const auto Qkj = Q(k, j); // Exchange ket

        auto& Dkj = D.block(k, j); // Exchange D
        const auto dkj_size = Dkj.size();
        const auto Dkj_norm = Dkj.norm();

        if (iter % num_nodes != this_node) {
          continue; // MPI distribute round robin
        }

        // for (auto l : sparse_list[k]) {
        // #pragma omp parallel for
        for (auto l = 0; l < nwaters; ++l) {
          const auto Qkl = Q(k, l); // Coulomb ket
          const auto Qil = Q(i, l); // Exchange bra

          auto& Dkl = D.block(k, l); // Coulomb D
          const auto dkl_size = Dkl.size();

          scf::MatrixBlock Fij_local(
            scf::nbasis_per_water, scf::nbasis_per_water);
          Fij_local.setZero();

          scf::MatrixBlock Fil_local(
            scf::nbasis_per_water, scf::nbasis_per_water);
          Fil_local.setZero();

          if (do_J(Qij, Qkl, Dkl.norm()) || do_K(Qil, Qkj, Dkj_norm)) {

            const auto tid = omp_get_thread_num();
            auto& eng = engines[tid];
            // We may want to dive down into this and do shell level screening
            auto ij_kl_ints = scf::two_e_integral(
              waters[i], waters[j], waters[k], waters[l], eng);

            for (auto p = 0, pqrs = 0; p < scf::nbasis_per_water; ++p) {
              for (auto q = 0; q < scf::nbasis_per_water; ++q) {
                const auto pq = p * scf::nbasis_per_water + q;

                for (auto r = 0; r < scf::nbasis_per_water; ++r) {
                  for (auto s = 0; s < scf::nbasis_per_water; ++s, ++pqrs) {
                    const auto rs = r * scf::nbasis_per_water + s;

                    const auto val = ij_kl_ints(pq, rs);
                    if (dkl_size) { // This cleanly maps to matrix vector
                      Fij_local(p, q) += 2 * Dkl(r, s) * val;
                    }
                    if (dkj_size) { // This less cleanly maps to matrix vector
                      Fil_local(p, s) -= Dkj(r, q) * val;
                    }
                  }
                }
              }
            }
          }

          auto Fil = &F.block(i, l);

          // #pragma omp critical
          if (Fij->size() == 0) {
            Fij->resize(scf::nbasis_per_water, scf::nbasis_per_water);
            Fij->setZero();
          }

          if (Fil->size() == 0) {
            Fil->resize(scf::nbasis_per_water, scf::nbasis_per_water);
            Fil->setZero();
          }

          (*Fij) += Fij_local;
          (*Fil) += Fil_local;
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

  eng.set_precision(0.0);

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
          assert(buf_1234 != nullptr);

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

  eng.set_precision(0.0);
  auto const& buf = eng.results();

  auto s1_start = 0;
  for (auto const& s1 : i.shells()) {
    const auto s1_size = s1.size();

    auto s2_start = 0;
    for (auto const& s2 : j.shells()) {
      const auto s2_size = s2.size();
      eng.compute(s1, s2);

      Eigen::Map<const MatrixBlock> buf_map(buf[0], s1_size, s2_size);
      output.block(s1_start, s2_start, s1_size, s2_size) = buf_map;

      s2_start += s2_size;
    }
    s1_start += s1_size;
  }

  return output;
}

scf::MatrixBlock scf::four_center_update(
  scf::MatrixBlock const& D, scf::Water const& w, libint2::Engine& eng) {

  scf::MatrixBlock F(scf::nbasis_per_water, scf::nbasis_per_water);
  F.setZero();
  auto ij_kl_ints = scf::two_e_integral(w, w, w, w, eng);

  for (auto p = 0, pqrs = 0; p < scf::nbasis_per_water; ++p) {
    for (auto q = 0; q < scf::nbasis_per_water; ++q) {
      const auto pq = p * scf::nbasis_per_water + q;

      for (auto r = 0; r < scf::nbasis_per_water; ++r) {
        for (auto s = 0; s < scf::nbasis_per_water; ++s, ++pqrs) {
          const auto rs = r * scf::nbasis_per_water + s;

          const auto val = ij_kl_ints(pq, rs);
          F(p, q) += 2 * D(r, s) * val;
          F(p, s) -= D(r, q) * val;
        }
      }
    }
  }

  return F;
}


scf::MatrixBlock scf::matrixBasedScf(scf::Water const& w) {
  const auto max_nprim = 6;
  const auto max_ang = 1;

  libint2::Engine eng_o(libint2::Operator::overlap, max_nprim, max_ang, 0);
  libint2::Engine eng_t(libint2::Operator::kinetic, max_nprim, max_ang, 0);
  libint2::Engine eng_v(libint2::Operator::nuclear, max_nprim, max_ang, 0);

  eng_v.set_params(scf::nuclei_charges({w}));

  auto S = one_body_integral(w, w, eng_o);
  auto T = one_body_integral(w, w, eng_t);
  auto V = one_body_integral(w, w, eng_v);
  scf::MatrixBlock H = T + V;
  T.resize(0, 0);
  V.resize(0, 0);

  scf::MatrixBlock F = H;

  auto new_density = [](auto F, auto S) {
    Eigen::GeneralizedSelfAdjointEigenSolver<scf::MatrixBlock> gevd(F, S);
    scf::MatrixBlock const& C = gevd.eigenvectors().leftCols(5);
    return scf::MatrixBlock(C * C.transpose());
  };

  scf::MatrixBlock D = new_density(F, S);

  libint2::Engine eng_2e(libint2::Operator::coulomb, max_nprim, max_ang, 0);
  eng_2e.set_precision(0.0);

  auto energy = 0.0;
  auto diff = 1.0;
  auto iter = 0;
  libint2::DIIS<scf::MatrixBlock> diis(2); // start DIIS on second iteration
  while (diff > 1e-8) {
    F = H;
    F += four_center_update(D, w, eng_2e);
    auto new_energy = D.cwiseProduct(H + F).sum();

    scf::MatrixBlock grad = F * D * S - S * D * F;
    scf::MatrixBlock Fdiis = F;

    diis.extrapolate(Fdiis, grad);

    D = new_density(Fdiis, S);

    diff = std::abs(new_energy - energy);
    energy = new_energy;
  }

  return D;
}

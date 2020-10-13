/*
//@HEADER
// *****************************************************************************
//
//                            runtime_diagnostics.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/config.h"

#if vt_check_enabled(diagnostics)

#include "vt/runtime/runtime.h"
#include "vt/scheduler/scheduler.h"
#include "vt/runtime/component/diagnostic_enum_format.h"
#include "vt/runtime/component/diagnostic_value_format.h"
#include "vt/runtime/component/diagnostic_units.h"
#include "vt/runtime/component/diagnostic_erased_value.h"

#if vt_check_enabled(libfort)
#include <fort.hpp>
#endif /*vt_check_enabled(libfort)*/

#include <fmt/format.h>

#include <map>
#include <string>
#include <fstream>

namespace vt { namespace runtime {

/**
 * Helpers for formatting values for printing
 */
namespace {

struct FormatHelper {
  FormatHelper(component::DiagnosticUnit in_unit, bool in_align, bool in_base)
    : unit_(in_unit),
      align_(in_align),
      base_(in_base)
  { }

  template <typename T>
  std::string apply(
    typename component::DiagnosticErasedValue::UnionValueType eval
  ) {
    using DF = component::detail::DiagnosticFormatter;
    using component::detail::decimal_format;

    bool const is_decimal =
      std::is_same<T, float>::value or std::is_same<T, double>::value;

    std::string default_format = is_decimal ? decimal_format : std::string{"{}"};

    if (base_) {
      return fmt::format(default_format, eval.get<T>());
    } else {
      return DF::getValueWithUnits(eval.get<T>(), unit_, default_format, align_);
    }
  }

  component::DiagnosticUnit unit_;
  bool align_ = true;
  bool base_ = false;
};

template <>
std::string FormatHelper::apply<void>(
  typename component::DiagnosticErasedValue::UnionValueType
) {
  vtAssert(false, "Failed to extract type from union");
  return "";
}

std::string valueFormatHelper(
  typename component::DiagnosticErasedValue::UnionValueType eval,
  component::DiagnosticUnit unit,
  bool align = true,
  bool base = false
) {
  FormatHelper fn(unit, align, base);
  return eval.switchOn(fn);
}

std::string valueFormatHelper(
  double val, component::DiagnosticUnit unit, bool align = true,
  bool base = false
) {
  using DF = component::detail::DiagnosticFormatter;
  using component::detail::decimal_format;

  if (base) {
    return fmt::format(decimal_format, val);
  } else {
    return DF::getValueWithUnits(val, unit, decimal_format, align);
  }
}

} /* end anon namespace */

/**
 * Helpers for outputting diagnostics to the screen or file by iterating through
 * them and applying a function
 */
namespace {

using ComponentDiagnosticMap = std::map<
  component::detail::DiagnosticBase*,
  std::unique_ptr<component::DiagnosticErasedValue>
>;

void foreachDiagnosticValue(
  std::map<std::string, ComponentDiagnosticMap> const& vals,
  std::function<void(bool first, std::string const& conponent)> sep,
  std::function<void(
    std::string const& component, component::detail::DiagnosticBase*,
    component::DiagnosticErasedValue*
  )> fn
) {
  for (auto&& elm : vals) {
    auto comp = elm.first;
    auto& map = elm.second;
    bool first = true;
    for (auto&& diag_elm : map) {
      auto diag = diag_elm.first;
      auto& str = diag_elm.second;

      // skip invalid/sentinel values unless it's a Sum, which might be useful
      // for analysis to inform that the value is zero
      if (
        not str->is_valid_value_ and
        not (str->update_ == component::DiagnosticUpdate::Sum)
      ) {
        continue;
      }

      if (sep) {
        sep(first, comp);
      }
      if (first) {
        first = false;
      }

      fn(comp, diag, str.get());
    }
  }
}

} /* end anon namespace */

void Runtime::computeAndPrintDiagnostics() {

  std::map<std::string, ComponentDiagnosticMap> component_vals;

  runInEpochCollective([&]{
    p_->foreach([&](component::BaseComponent* c) {
      // Run the pre-diagnostic hook
      c->preDiagnostic();
      c->foreachDiagnostic([&](component::detail::DiagnosticBase* d) {
        component_vals[c->name()][d] = std::make_unique<
          component::DiagnosticErasedValue
        >();
        d->reduceOver(c, component_vals[c->name()][d].get(), 0);
      });
    });
  });

  if (theContext->getNode() not_eq 0) {
    return;
  }

  /// Builder function for making a histogram for output
  auto make_hist =
    [](adt::HistogramApprox<double, int64_t>& h, char delim) -> std::string {
      auto buckets = h.computeFixedBuckets(8);
      std::string hist_out = fmt::format("<");
      for (std::size_t i = 0; i < buckets.size(); i++) {
        hist_out += fmt::format("{}", buckets[i]);
        if (i == buckets.size() - 1) {
          hist_out += ">";
        } else {
          hist_out += std::string{delim} + " ";
        }
      }
      return hist_out;
    };

# if vt_check_enabled(libfort)
  if (
    theConfig()->vt_diag_print_summary or
    theConfig()->vt_diag_summary_file != ""
  ) {
    fort::utf8_table table;
    table.set_border_style(FT_BOLD_STYLE); //FT_BOLD2_STYLE
    table << fort::header
          << "Component"
          << "Metric"
          << "Description"
          << "Type"
          << "Total"
          << "Mean"
          << "Min"
          << "Max"
          << "Std"
          << "Histogram"
          << fort::endr;
    //table.row(0).set_cell_content_text_style(fort::text_style::underlined);

    table.row(0).set_cell_content_text_style(fort::text_style::bold);
    table.row(0).set_cell_text_align(fort::text_align::center);
    table.column(0).set_cell_text_align(fort::text_align::center);

    foreachDiagnosticValue(
      component_vals,
      [&](bool first, std::string const& comp){
        if (first) {
          table << fort::separator;
          table[table.cur_row()][0].set_cell_content_fg_color(fort::color::red);
          table << comp;
        } else {
          table[table.cur_row()][0].set_cell_content_fg_color(fort::color::red);
          table << "";
        }
      },
      [&](
        std::string const& comp, component::detail::DiagnosticBase* diag,
        component::DiagnosticErasedValue* str
      ) {
        table[table.cur_row()][1].set_cell_content_fg_color(fort::color::green);
        table[table.cur_row()][2].set_cell_content_fg_color(fort::color::blue);

        // Right-align the number cells
        for (int i = 4; i <= 9; i++) {
          table[table.cur_row()][i].set_cell_text_align(fort::text_align::right);
        }

        auto update = diag->getUpdateType();

        // If not displaying a value in the column, center the spacer
        if (not diagnosticShowTotal(update)) {
          table[table.cur_row()][4].set_cell_text_align(fort::text_align::center);
        }

        auto hist_out = make_hist(str->hist_, ',');

        table
          << diag->getKey()
          << diag->getDescription()
          << diagnosticUpdateTypeString(update)
          << (diagnosticShowTotal(update) ?
              valueFormatHelper(str->sum_, str->unit_) : std::string("--"))
          << valueFormatHelper(str->avg_, str->unit_)
          << valueFormatHelper(str->min_, str->unit_)
          << valueFormatHelper(str->max_, str->unit_)
          << valueFormatHelper(str->std_, str->unit_)
          << hist_out
          << fort::endr;
        //fmt::print("component={}; name={}; value={}\n", comp, diag->getKey(), str->avg_value_);

      }
    );

    auto out = fmt::format("{}", table.to_string());

    if (theConfig()->vt_diag_print_summary) {
      fmt::print(out);
    }

    if (theConfig()->vt_diag_summary_file != "") {
      std::ofstream fout;
      fout.open(
        theConfig()->vt_diag_summary_file, std::ios::out | std::ios::binary
      );
      vtAssert(fout.good(), "Must be a valid file");
      fout << out;
      fout.close();

      auto green    = debug::green();
      auto reset    = debug::reset();
      auto bd_green = debug::bd_green();
      auto magenta  = debug::magenta();
      auto f1 = fmt::format("{}Diagnostic table written out to: {}", green, reset);
      auto vt_pre = bd_green + std::string("vt") + reset + ": ";
      fmt::print(
        "{}{}{}{}{}\n",
        vt_pre, f1, magenta, theConfig()->vt_diag_summary_file, reset
      );
    }
  }
# endif /*vt_check_enabled(libfort)*/

  if (theConfig()->vt_diag_summary_csv_file != "") {
    std::string out;
    out += fmt::format(
      "Component, "
      "Metric, "
      "Description, "
      "Type, "
      "Total, "
      "Mean, "
      "Min, "
      "Max, "
      "Std, "
      "Histogram\n"
    );

    foreachDiagnosticValue(
      component_vals,
      nullptr,
      [&](
        std::string const& comp, component::detail::DiagnosticBase* diag,
        component::DiagnosticErasedValue* str
      ) {
        auto hist_out = make_hist(str->hist_, ';');
        auto update = diag->getUpdateType();
        auto base = theConfig()->vt_diag_csv_base_units;

        out += fmt::format(
          "{},{},{},{},{},{},{},{},{},{}\n",
          comp,
          diag->getKey(),
          diag->getDescription(),
          diagnosticUpdateTypeString(update),
          (diagnosticShowTotal(update) ?
           valueFormatHelper(str->sum_, str->unit_, false, base) :
           std::string("--")
          ),
          valueFormatHelper(str->avg_, str->unit_, false, base),
          valueFormatHelper(str->min_, str->unit_, false, base),
          valueFormatHelper(str->max_, str->unit_, false, base),
          valueFormatHelper(str->std_, str->unit_, false, base),
          hist_out
        );
      }
    );

    std::ofstream fout;
    fout.open(
      theConfig()->vt_diag_summary_csv_file, std::ios::out | std::ios::binary
    );
    vtAssert(fout.good(), "Must be a valid file");
    fout << out;
    fout.close();

    auto green    = debug::green();
    auto reset    = debug::reset();
    auto bd_green = debug::bd_green();
    auto magenta  = debug::magenta();
    auto f1 = fmt::format("{}Diagnostic CSV written out to: {}", green, reset);
    auto vt_pre = bd_green + std::string("vt") + reset + ": ";
    fmt::print(
      "{}{}{}{}{}\n",
      vt_pre, f1, magenta, theConfig()->vt_diag_summary_csv_file, reset
    );
  }
}

}} /* end namespace vt::runtime */

#endif /* vt_check_enabled(diagnostics) */

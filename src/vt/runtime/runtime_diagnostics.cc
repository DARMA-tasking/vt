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

#include "vt/runtime/runtime.h"
#include "vt/scheduler/scheduler.h"
#include "vt/runtime/component/diagnostic_enum_format.h"
#include "vt/runtime/component/diagnostic_value_format.h"
#include "vt/runtime/component/diagnostic_units.h"
#include "vt/runtime/component/diagnostic_erased_value.h"

#include <fort.hpp>

#include <fmt/format.h>

#include <map>
#include <string>

namespace vt { namespace runtime {

namespace {

std::string valueFormatHelper(double val, component::DiagnosticUnit unit) {
  using DF = component::detail::DiagnosticFormatter;
  using component::detail::decimal_format;
  return DF::getValueWithUnits(val, unit, decimal_format);
}

template <typename T>
struct FormatHelper {
  std::string operator()(
    typename component::DiagnosticErasedValue::UnionValueType eval,
    component::DiagnosticUnit unit
  ) {
    using DF = component::detail::DiagnosticFormatter;
    using component::detail::decimal_format;

    bool const is_decimal =
      std::is_same<T, float>::value or std::is_same<T, double>::value;

    std::string default_format = is_decimal ? decimal_format : std::string{"{}"};

    return DF::getValueWithUnits(eval.get<T>(), unit, default_format);
  }
};

template <>
struct FormatHelper<void> {
  std::string operator()(
    typename component::DiagnosticErasedValue::UnionValueType,
    component::DiagnosticUnit
  ) {
    vtAssert(false, "Failed to extract type from union");
    return "";
  }
};

std::string valueFormatHelper(
  typename component::DiagnosticErasedValue::UnionValueType eval,
  component::DiagnosticUnit unit
) {
  return eval.template switchOn<FormatHelper>(eval, unit);
}

} /* end anon namespace */

void Runtime::computeAndPrintDiagnostics() {

  using ComponentDiagnosticMap = std::map<
    component::detail::DiagnosticBase*,
    std::unique_ptr<component::DiagnosticErasedValue>
  >;

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

  if (theContext->getNode() == 0) {
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
          << fort::endr;
    //table.row(0).set_cell_content_text_style(fort::text_style::underlined);

    table.row(0).set_cell_content_text_style(fort::text_style::bold);
    table.row(0).set_cell_text_align(fort::text_align::center);
    table.column(0).set_cell_text_align(fort::text_align::center);

    for (auto&& elm : component_vals) {
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

        if (first) {
          table[table.cur_row()][0].set_cell_content_fg_color(fort::color::red);
          table << comp;
        } else {
          table << "";
        }
        first = false;

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
          << fort::endr;
        //fmt::print("component={}; name={}; value={}\n", comp, diag->getKey(), str->avg_value_);

      }
      table << fort::separator;
    }

    fmt::print("{}", table.to_string());
  }
}

}} /* end namespace vt::runtime */

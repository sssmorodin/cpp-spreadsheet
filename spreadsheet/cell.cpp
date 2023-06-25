#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

// Реализуйте следующие методы
Cell::Cell(const SheetInterface& sheet)
    : sheet_(sheet) {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() {
    Clear();
}

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (1 == text.size()) {
        impl_ = std::make_unique<TextImpl>(text);
    } else {
        char first = text[0];
        switch (first) {
            case FORMULA_SIGN:
                impl_ = std::make_unique<FormulaImpl>(text.substr(1, text.size() - 1), sheet_);
                referenced_cells_ptrs_ = dynamic_cast<FormulaImpl*>(impl_.get())->GetFormulaPtr()->GetReferencedCells();
                break;
            default:
                impl_ = std::make_unique<TextImpl>(text);
        }
    }
}

void Cell::Clear() {
    impl_.release();
}

Cell::Value Cell::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = impl_->GetValue();
    }
    return cache_.value();
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return referenced_cells_ptrs_;
}

std::string EmptyImpl::GetText() const {
    return "";
}
CellInterface::Value EmptyImpl::GetValue() const {
    return "";
}

TextImpl::TextImpl(std::string text)
        : text_(text) {
}
std::string TextImpl::GetText() const {
    return text_;
}
CellInterface::Value TextImpl::GetValue() const {
    if (ESCAPE_SIGN == text_[0]) {
        return text_.substr(1, text_.size() - 1);
    } else {
        return text_;
    }
}

FormulaImpl::FormulaImpl(std::string text, const SheetInterface& sheet)
        : value_(ParseFormula(text))
        , sheet_(sheet) {
}

std::string FormulaImpl::GetText() const {
    return FORMULA_SIGN + value_->GetExpression();
}

CellInterface::Value FormulaImpl::GetValue() const {
    auto result = value_->Evaluate(sheet_);
    if (const double* doubleValue = std::get_if<double>(&result)) {
        return *doubleValue;  // Implicitly converted to CellInterface::Value (double)
    } else if (const FormulaError* errorValue = std::get_if<FormulaError>(&result)) {
        return *errorValue;  // Implicitly converted to CellInterface::Value (FormulaError)
    } else {
        throw std::logic_error("Invalid FormulaInterface::Value");
    }
}

const FormulaInterface* FormulaImpl::GetFormulaPtr() const {
    return value_.get();
}

void Cell::AddDependentCell(Position pos) {
    dependent_cells_.insert(pos);
}

const std::unordered_set<Position, PositionHasher> Cell::GetDependentCells() const {
    return dependent_cells_;
}

std::optional<Cell::Value>& Cell::GetCache() {
    return cache_;
}

void Cell::DeleteDependentCell(Position dependent_cell) {
    dependent_cells_.erase(dependent_cell);
}
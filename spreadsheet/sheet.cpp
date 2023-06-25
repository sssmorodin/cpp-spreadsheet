#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <exception>

using namespace std::literals;

Sheet::~Sheet() {
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPositionException");
    }
    std::unique_ptr<Cell> cell = std::make_unique<Cell>(std::ref(*this));
    cell->Set(text);

    // cache invalidation, ref_cells dependancies deleting
    if (sheet_.count(pos) && !sheet_.at(pos)->GetDependentCells().empty()) {
        InvalidateCachesChain(pos, sheet_.at(pos)->GetDependentCells());
        for (const Position& current_ref_cell : sheet_.at(pos)->GetReferencedCells()) {
            static_cast<Cell*>(GetCell(current_ref_cell))->DeleteDependentCell(pos);
        }
    }

    if (!HasCircularDependency(pos, cell->GetReferencedCells())) {
        sheet_[pos] = std::move(cell);
        row_to_nonempty_col_[pos.row].insert(pos.col);
    } else {
        throw CircularDependencyException("");
    }

    if (sheet_size_.rows < pos.row + 1) {
        sheet_size_.rows = pos.row + 1;
    }
    if (sheet_size_.cols < pos.col + 1) {
        sheet_size_.cols = pos.col + 1;
    }

    // if the Cell was successfully added to a Sheet
    if (0 != sheet_.count(pos)) {
        for (const Position& ref_cell : sheet_.at(pos)->GetReferencedCells()) {
            if (0 == sheet_.count(ref_cell)) {
                SetCell(ref_cell, "");
            }
            sheet_.at(ref_cell)->AddDependentCell(pos);
        }
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPositionException");
    }

    if (Position{sheet_size_.rows - 1, sheet_size_.cols - 1} < pos) {
        return nullptr;
    }
    return sheet_.count(pos) ? sheet_.at(pos).get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPositionException");
    }

    if (Position{sheet_size_.rows - 1, sheet_size_.cols - 1} < pos) {
        return nullptr;
    }
    return sheet_.count(pos) ? sheet_.at(pos).get() : nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPositionException");
    }
    Size sheet_size = GetPrintableSize();
    if (Position{sheet_size.rows - 1, sheet_size.cols - 1} < pos) {
        return;
    }
    if (0 == sheet_.count(pos) || nullptr == sheet_.at(pos).get()) {
        return;
    }

    static_cast<Cell*>(GetCell(pos))->Clear();

    // cache invalidation, ref_cells dependancies deleting
    if (sheet_.count(pos) && !sheet_.at(pos)->GetDependentCells().empty()) {
        InvalidateCachesChain(pos, sheet_.at(pos)->GetDependentCells());
        for (const Position& current_ref_cell : sheet_.at(pos)->GetReferencedCells()) {
            static_cast<Cell*>(GetCell(current_ref_cell))->DeleteDependentCell(pos);
        }
    }
    sheet_.at(pos).release();

    row_to_nonempty_col_.at(pos.row).erase(pos.col);
    if (row_to_nonempty_col_.at(pos.row).empty()) {
        row_to_nonempty_col_.erase(pos.row);
    }

    // new printable area calculation
    if (row_to_nonempty_col_.empty()) {
        sheet_size_.rows = 0;
        sheet_size_.cols = 0;
        return;
    }
    if (sheet_size.rows - 1 == pos.row && 0 == row_to_nonempty_col_.count(pos.row)) {
        int max_key = 0;
        for (const auto& [key, _] : row_to_nonempty_col_) {
            if (key > max_key) {
                max_key = key;
            }
        }
        sheet_size_.rows = max_key + 1;
    }
    if (sheet_size.cols - 1 == pos.col) {
        int max_col = 0;
        for (const auto& [row, cols] : row_to_nonempty_col_) {
            if (row_to_nonempty_col_.at(row).count(pos.col)) {
                break;
            }
            if (!cols.empty()) {
                auto max_curr_col = std::max_element(cols.begin(), cols.end());
                if (max_col < *max_curr_col) {
                    max_col = *max_curr_col;
                }
            }
        }
        sheet_size_.cols = max_col + 1;
    }
}

Size Sheet::GetPrintableSize() const {
    return sheet_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for (int i = 0; i < printable_size.rows; ++i) {
        bool is_first_in_row = true;
        for (int j = 0; j < printable_size.cols; ++j) {
            if (!is_first_in_row) {
                output << '\t';
            }
            is_first_in_row = false;
            if (sheet_.count(Position{i, j}) && nullptr != sheet_.at(Position{i, j})) {
                std::visit([&output](const auto& value) {
                    output << value;
                }, sheet_.at(Position{i, j})->GetValue());
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for (int i = 0; i < printable_size.rows; ++i) {
        bool is_first_in_row = true;
        for (int j = 0; j < printable_size.cols; ++j) {
            if (!is_first_in_row) {
                output << '\t';
            }
            is_first_in_row = false;
            if (0 != sheet_.count(Position{i, j}) && nullptr != sheet_.at(Position{i, j})) {
                output << sheet_.at(Position{i, j})->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

bool Sheet::HasCircularDependency(Position pos, const std::vector<Position>& ref_cells) const {
    std::unordered_set<Position, PositionHasher> visited;
    return DeepFirstSearch(pos, ref_cells, visited);
}

bool Sheet::DeepFirstSearch(Position pos, const std::vector<Position>& referenced_cells, std::unordered_set<Position, PositionHasher>& visited) const {
    visited.insert(pos);
    if (referenced_cells.empty()) {
        return false;
    }
    for (const Position& ref_pos : referenced_cells) {
        if (visited.count(ref_pos) != 0) {
            return true;
        }
        if (nullptr != GetCell(ref_pos)) {
            if (DeepFirstSearch(ref_pos, GetCell(ref_pos)->GetReferencedCells(), visited)) {
                return true;
            }
        }
    }
    return false;
}

void Sheet::InvalidateCachesChain(Position pos, const std::unordered_set<Position, PositionHasher>& dependent_cells) {
    std::unordered_set<Position, PositionHasher> visited;
    return ClearCash(pos, dependent_cells, visited);
}

void Sheet::ClearCash(Position pos, const std::unordered_set<Position, PositionHasher>& dependent_cells, std::unordered_set<Position, PositionHasher>& visited) {
    std::optional<Cell::Value>& current_cell_cache = static_cast<Cell*>(GetCell(pos))->GetCache();
    if (current_cell_cache.has_value()) {
        current_cell_cache.reset();
    }
    visited.insert(pos);
    if (dependent_cells.empty()) {
        return;
    }
    for (const Position& ref_pos : dependent_cells) {
        if (visited.count(ref_pos) != 0) {
            return;
        }
        if (nullptr != GetCell(ref_pos)) {
            ClearCash(ref_pos, static_cast<Cell*>(GetCell(ref_pos))->GetDependentCells(), visited);
        }
    }
}

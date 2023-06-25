#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <algorithm>

class Sheet : public SheetInterface {
public:
    Sheet() = default;
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    bool HasCircularDependency(Position pos, const std::vector<Position>& ref_cells) const;
    bool DeepFirstSearch(Position pos, const std::vector<Position>& referenced_cells, std::unordered_set<Position, PositionHasher>& visited) const;

    void InvalidateCachesChain(Position pos, const std::unordered_set<Position, PositionHasher>& dependent_cells);
    void ClearCash(Position pos, const std::unordered_set<Position, PositionHasher>& dependent_cells, std::unordered_set<Position, PositionHasher>& visited);

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> sheet_;
    // indexes in max_significant_index_in_row_ starts from 1
    std::unordered_map<int, std::unordered_set<int>> row_to_nonempty_col_;
    //std::vector<int> max_significant_index_in_row_;
    Size sheet_size_;
};
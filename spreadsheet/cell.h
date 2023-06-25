#pragma once

#include <optional>
#include <variant>
#include <unordered_set>

#include "common.h"
#include "formula.h"

// base class
class Impl {
public:
    Impl() = default;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
};

class EmptyImpl : public Impl {
public:
    EmptyImpl() = default;
    std::string GetText() const override;
    CellInterface::Value GetValue() const override;
};

class TextImpl : public Impl {
public:
    TextImpl(std::string text);
    std::string GetText() const override;
    CellInterface::Value GetValue() const override;
private:
    std::string text_;
};

class FormulaImpl : public Impl {
public:
    FormulaImpl(std::string text, const SheetInterface& sheet);
    std::string GetText() const override;
    CellInterface::Value GetValue() const override;
    const FormulaInterface* GetFormulaPtr() const;
private:
    std::unique_ptr<FormulaInterface> value_;
    const SheetInterface& sheet_;
};

class Cell : public CellInterface {
public:
    Cell(const SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    void AddDependentCell(Position pos);
    const std::unordered_set<Position, PositionHasher> GetDependentCells() const;
    std::optional<Cell::Value>& GetCache();
    void DeleteDependentCell(Position dependent_cell);

private:
    std::unique_ptr<Impl> impl_;
    std::vector<Position> referenced_cells_ptrs_;
    std::unordered_set<Position, PositionHasher> dependent_cells_;
    mutable std::optional<Cell::Value> cache_;
    const SheetInterface& sheet_;
};
#include "formula.h"
#include "sheet.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
        : ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> lambda = [&sheet](Position pos) {
            double out = 0.0;
            const CellInterface* cell_ptr = sheet.GetCell(pos);
            if (nullptr != cell_ptr) {
                CellInterface::Value value = cell_ptr->GetValue();
                if (std::holds_alternative<double>(value)) {
                    out = std::get<double>(value);
                } else if (std::holds_alternative<std::string>(value)) {
                    std::string str_value = std::get<std::string>(value);
                    if (str_value.empty()) {
                        return 0.0;
                    }
                    std::size_t dbl_length = 0;
                    double dbl_value = std::stod(std::get<std::string>(value), &dbl_length);
                    if (str_value.length() == dbl_length) {
                        out = dbl_value;
                    } else {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }
            }
            return out;
        };

        try {
            return ast_.Execute(lambda);
        } catch (const std::invalid_argument&) {
            return FormulaError(FormulaError::Category::Value);
        } catch (const FormulaError& error) {
                return error;
        }
    }

    std::string GetExpression() const override {
        std::stringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const {
        std::vector<Position> out;
        for (const auto& cell_ptr : ast_.GetCells()) {
            out.push_back(cell_ptr);
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
        return out;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("");
    }
}
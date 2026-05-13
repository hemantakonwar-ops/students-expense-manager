#include "../include/Analytics.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>

// ── Constructor ───────────────────────────────────────────────────────────────

Analytics::Analytics(const BudgetConfig& budget)
    : m_budget(budget) {}

// ── total ─────────────────────────────────────────────────────────────────────

double Analytics::total(const std::vector<Expense>& expenses) const {
    double sum = 0.0;
    for (const auto& e : expenses) sum += e.amount;
    return sum;
}

// ── summarise ─────────────────────────────────────────────────────────────────

std::vector<CategorySummary> Analytics::summarise(const std::vector<Expense>& expenses) const {
    std::map<Category, CategorySummary> acc;

    // Initialise all categories
    for (auto cat : { Category::Food, Category::Transport,
                      Category::Entertainment, Category::Utilities, Category::Other }) {
        acc[cat] = { cat, 0.0, m_budget.limitFor(cat), false, 0 };
    }

    for (const auto& e : expenses) {
        acc[e.category].total += e.amount;
        acc[e.category].count += 1;
    }

    for (auto& [cat, s] : acc) {
        s.overBudget = (s.total > s.budget);
    }

    std::vector<CategorySummary> result;
    for (auto& [cat, s] : acc) result.push_back(s);

    // Sort by total descending
    std::sort(result.begin(), result.end(),
              [](const CategorySummary& a, const CategorySummary& b) {
                  return a.total > b.total;
              });

    return result;
}

// ── filterByMonth ─────────────────────────────────────────────────────────────

std::vector<Expense> Analytics::filterByMonth(const std::vector<Expense>& expenses,
                                               const std::string& month) const {
    std::vector<Expense> out;
    for (const auto& e : expenses) {
        if (e.date.size() >= 7 && e.date.substr(0, 7) == month)
            out.push_back(e);
    }
    return out;
}

// ── filterByRange ─────────────────────────────────────────────────────────────

std::vector<Expense> Analytics::filterByRange(const std::vector<Expense>& expenses,
                                               const std::string& from,
                                               const std::string& to) const {
    std::vector<Expense> out;
    for (const auto& e : expenses) {
        if (e.date >= from && e.date <= to)
            out.push_back(e);
    }
    return out;
}

// ── printBar ──────────────────────────────────────────────────────────────────

void Analytics::printBar(double spent, double limit) const {
    int pct    = (limit > 0) ? static_cast<int>(spent / limit * 100.0) : 100;
    int filled = std::min(pct / 5, 20);   // 20-char bar, each cell = 5%
    std::cout << "[";
    for (int i = 0; i < 20; ++i)
        std::cout << (i < filled ? '#' : '.');
    std::cout << "] " << std::setw(3) << pct << "%";
}

// ── printSummary ──────────────────────────────────────────────────────────────

void Analytics::printSummary(const std::vector<Expense>& expenses) const {
    if (expenses.empty()) {
        std::cout << "  No expenses to analyse.\n";
        return;
    }

    auto summaries = summarise(expenses);

    std::cout << "\n";
    std::cout << "  ─── MONTHLY ANALYTICS ───────────────────────────────────────\n";
    std::cout << std::left
              << "  " << std::setw(16) << "CATEGORY"
              << std::right
              << std::setw(9)  << "SPENT"
              << std::setw(9)  << "BUDGET"
              << "   STATUS   BAR\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    for (const auto& s : summaries) {
        std::cout << "  " << std::left << std::setw(16) << categoryToString(s.category)
                  << std::right
                  << std::setw(8) << std::fixed << std::setprecision(2) << s.total
                  << std::setw(9) << s.budget
                  << "   ";

        if (s.overBudget)
            std::cout << "\033[31m[OVER]\033[0m  ";   // red
        else
            std::cout << "\033[32m[OK]  \033[0m  ";   // green

        printBar(s.total, s.budget);
        std::cout << "  (" << s.count << " txn)\n";
    }

    std::cout << "  " << std::string(70, '-') << "\n";
    std::cout << "  " << std::left << std::setw(16) << "TOTAL"
              << std::right << std::setw(8) << std::fixed << std::setprecision(2)
              << total(expenses) << "\n\n";

    // ── Alerts ────────────────────────────────────────────────────────────────
    bool anyAlert = false;
    for (const auto& s : summaries) {
        double pct = (s.budget > 0) ? (s.total / s.budget * 100.0) : 100.0;
        if (s.overBudget) {
            std::cout << "  \033[31m[!] OVERSPEND: " << categoryToString(s.category)
                      << " exceeded budget by ₹"
                      << std::fixed << std::setprecision(2) << (s.total - s.budget)
                      << "\033[0m\n";
            anyAlert = true;
        } else if (pct >= 80.0) {
            std::cout << "  \033[33m[~] WARNING: " << categoryToString(s.category)
                      << " is at " << static_cast<int>(pct) << "% of budget\033[0m\n";
            anyAlert = true;
        }
    }
    if (anyAlert) std::cout << "\n";
}

// ── buildPromptSummary ────────────────────────────────────────────────────────

std::string Analytics::buildPromptSummary(const std::vector<Expense>& expenses) const {
    if (expenses.empty()) return "No expense data available.";

    auto summaries = summarise(expenses);
    double grand   = total(expenses);

    std::ostringstream oss;
    oss << "Monthly expense summary:\n";
    oss << "Total spent: ₹" << std::fixed << std::setprecision(2) << grand << "\n\n";
    oss << "Category breakdown:\n";

    for (const auto& s : summaries) {
        double pct = (s.budget > 0) ? (s.total / s.budget * 100.0) : 0.0;
        oss << "- " << categoryToString(s.category)
            << ": ₹" << std::fixed << std::setprecision(2) << s.total
            << " / ₹" << s.budget << " budget"
            << " (" << static_cast<int>(pct) << "%)"
            << (s.overBudget ? " [OVER BUDGET]" : "")
            << ", " << s.count << " transactions\n";
    }

    oss << "\nRecent transactions:\n";
    int shown = 0;
    for (auto it = expenses.rbegin(); it != expenses.rend() && shown < 5; ++it, ++shown) {
        oss << "- " << it->date << " | " << it->description
            << " | ₹" << std::fixed << std::setprecision(2) << it->amount
            << " [" << it->categoryStr() << "]\n";
    }

    return oss.str();
}

// ── exportReport ──────────────────────────────────────────────────────────────

bool Analytics::exportReport(const std::vector<Expense>& expenses,
                              const std::string& filePath,
                              const std::string& aiInsight) const {
    std::ofstream f(filePath);
    if (!f.is_open()) {
        std::cerr << "  [!] Cannot open " << filePath << " for writing.\n";
        return false;
    }

    // Timestamp
    time_t now  = time(nullptr);
    tm*    lt   = localtime(&now);
    char   ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", lt);

    f << "=============================================================\n";
    f << "  STUDENT EXPENSE REPORT\n";
    f << "  Generated: " << ts << "\n";
    f << "=============================================================\n\n";

    // Expense table
    f << "ID    DATE         DESCRIPTION                  AMOUNT    CATEGORY\n";
    f << std::string(65, '-') << "\n";
    for (const auto& e : expenses) {
        f << std::left
          << std::setw(6)  << e.id
          << std::setw(13) << e.date
          << std::setw(29) << e.description.substr(0, 28)
          << std::right
          << std::setw(8)  << std::fixed << std::setprecision(2) << e.amount
          << "  " << std::left << e.categoryStr() << "\n";
    }
    f << std::string(65, '-') << "\n";
    f << std::right << std::setw(48) << "TOTAL"
      << std::setw(8) << std::fixed << std::setprecision(2) << total(expenses) << "\n\n";

    // Category summary
    f << "CATEGORY SUMMARY\n" << std::string(40, '-') << "\n";
    for (const auto& s : summarise(expenses)) {
        double pct = (s.budget > 0) ? (s.total / s.budget * 100.0) : 0.0;
        f << std::left << std::setw(16) << categoryToString(s.category)
          << "  ₹" << std::setw(8) << std::fixed << std::setprecision(2) << s.total
          << " / ₹" << s.budget
          << "  (" << static_cast<int>(pct) << "%)"
          << (s.overBudget ? "  *** OVER BUDGET ***" : "")
          << "\n";
    }

    // AI insight
    if (!aiInsight.empty()) {
        f << "\nAI ADVISOR INSIGHT\n" << std::string(40, '-') << "\n";
        f << aiInsight << "\n";
    }

    f << "\n=============================================================\n";
    std::cout << "  [✓] Report saved to: " << filePath << "\n\n";
    return true;
}
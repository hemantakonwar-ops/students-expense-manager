#include <limits>
#include <algorithm>
#include "../include/AIadviser.h"
#include "../include/Analytics.h"
#include "../include/expense.h"
#include "../include/Storage.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// ── Helpers ───────────────────────────────────────────────────────────────────

static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    (void)system("clear");
#endif
}

static void pause() {
    std::cout << "\n  Press Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static std::string currentMonth() {
    time_t now = time(nullptr);
    tm*    lt  = localtime(&now);
    char   buf[8];
    strftime(buf, sizeof(buf), "%Y-%m", lt);
    return buf;
}

static std::string previousMonth() {
    time_t now = time(nullptr);
    tm*    lt  = localtime(&now);
    lt->tm_mon -= 1;
    mktime(lt);
    char buf[8];
    strftime(buf, sizeof(buf), "%Y-%m", lt);
    return buf;
}

static std::string currentTimestamp() {
    time_t now = time(nullptr);
    tm*    lt  = localtime(&now);
    char   buf[20];
    strftime(buf, sizeof(buf), "%Y_%m", lt);
    return buf;
}

static int nextExpenseId(const std::vector<Expense>& expenses) {
    int maxId = 0;
    for (const auto& e : expenses) maxId = std::max(maxId, e.id);
    return maxId + 1;
}

// ── Ensure model is loaded ────────────────────────────────────────────────────

static bool ensureAdvisor(std::unique_ptr<AIAdvisor>& advisor) {
    if (!advisor) {
        std::cout << "  [~] Loading Phi-3-mini model (first use — may take 30s)...\n";
        advisor = std::make_unique<AIAdvisor>("models/Phi-3.1-mini-4k-instruct-Q4_K_M.gguf");
    }
    if (!advisor->isLoaded()) {
        std::cout << "  [!] Model unavailable. See README for download instructions.\n";
        return false;
    }
    return true;
}

static void streamQuery(AIAdvisor& advisor, const std::string& prompt) {
    std::cout << "  [~] Running inference (streaming output below):\n";
    std::cout << "  " << std::string(50, '-') << "\n\n";
    std::string result = advisor.query(prompt, nullptr);
    std::cout << result << std::flush;
    std::cout << "\n\n  " << std::string(50, '-') << "\n";
    std::cout << "  [✓] Inference complete.\n";
}

// ── Banner ────────────────────────────────────────────────────────────────────

static void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════════════════╗\n";
    std::cout << "  ║      STUDENT EXPENSE MANAGER  v1.0            ║\n";
    std::cout << "  ║      AI Advisor: Phi-3-mini                   ║\n";
    std::cout << "  ╚═══════════════════════════════════════════════╝\n\n";
}

static void printMenu() {
    std::cout << "  ┌─ MAIN MENU ────────────────────────────────────┐\n";
    std::cout << "  │  1. Add Expense                                │\n";
    std::cout << "  │  2. View All Expenses                          │\n";
    std::cout << "  │  3. View This Month's Expenses                 │\n";
    std::cout << "  │  4. Analytics & Budget Summary                 │\n";
    std::cout << "  │  5. Delete Expense                             │\n";
    std::cout << "  │  6. AI Financial Advisor                       │\n";
    std::cout << "  │  7. Export Report                              │\n";
    std::cout << "  │  0. Exit                                       │\n";
    std::cout << "  └────────────────────────────────────────────────┘\n";
    std::cout << "  Choice: ";
}

static void printAIMenu() {
    std::cout << "  ┌─ AI ADVISOR ───────────────────────────────────┐\n";
    std::cout << "  │  1. General money saving tips                  │\n";
    std::cout << "  │  2. Ask a custom question                      │\n";
    std::cout << "  │  3. Category-specific advice                   │\n";
    std::cout << "  │  4. Compare this month vs last month           │\n";
    std::cout << "  │  0. Back                                       │\n";
    std::cout << "  └────────────────────────────────────────────────┘\n";
    std::cout << "  Choice: ";
}

// ── Menu handlers ─────────────────────────────────────────────────────────────

static void handleAdd(std::vector<Expense>& expenses, Storage& storage) {
    std::cout << "\n  ─── Add New Expense ───────────────────────────\n\n";
    Expense e = promptNewExpense(nextExpenseId(expenses));
    expenses.push_back(e);

    StorageError err = storage.append(e);
    if (err.failed) {
        std::cerr << "  [!] Save error: " << err.message << "\n";
    } else {
        std::cout << "\n  [✓] Expense #" << e.id << " saved: "
                  << e.description << " ₹" << std::fixed << std::setprecision(2) << e.amount
                  << " [" << e.categoryStr() << "]\n";
    }
}

static void handleViewAll(const std::vector<Expense>& expenses) {
    std::cout << "\n  ─── All Expenses ──────────────────────────────\n";
    printExpenseTable(expenses);
}

static void handleViewMonth(const std::vector<Expense>& expenses) {
    Analytics analytics;
    std::string month = currentMonth();
    std::cout << "\n  ─── Expenses for " << month << " ──────────────────────\n";
    auto filtered = analytics.filterByMonth(expenses, month);
    printExpenseTable(filtered);
}

static void handleAnalytics(const std::vector<Expense>& expenses) {
    Analytics analytics;
    auto filtered = analytics.filterByMonth(expenses, currentMonth());
    std::cout << "\n  (Showing analytics for current month: " << currentMonth() << ")\n";
    if (filtered.empty()) {
        std::cout << "  No expenses this month. Showing all-time:\n";
        analytics.printSummary(expenses);
    } else {
        analytics.printSummary(filtered);
    }
}

static void handleDelete(std::vector<Expense>& expenses, Storage& storage) {
    if (expenses.empty()) {
        std::cout << "  No expenses to delete.\n";
        return;
    }

    std::cout << "\n  Enter expense ID to delete: ";
    std::string idStr;
    std::getline(std::cin, idStr);

    int id = 0;
    try { id = std::stoi(idStr); } catch (...) {}

    auto it = std::find_if(expenses.begin(), expenses.end(),
                           [id](const Expense& e) { return e.id == id; });

    if (it == expenses.end()) {
        std::cout << "  [!] Expense #" << id << " not found.\n";
        return;
    }

    std::cout << "  Delete: " << it->description
              << " ₹" << std::fixed << std::setprecision(2) << it->amount
              << "? (y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm == "y" || confirm == "Y") {
        expenses.erase(it);
        StorageError err = storage.save(expenses);
        if (err.failed)
            std::cerr << "  [!] Save error: " << err.message << "\n";
        else
            std::cout << "  [✓] Expense #" << id << " deleted.\n";
    } else {
        std::cout << "  Cancelled.\n";
    }
}

// ── AI sub-handlers ───────────────────────────────────────────────────────────

// 1. General tips based on full expense summary
static void aiGeneralTips(const std::vector<Expense>& expenses,
                           std::unique_ptr<AIAdvisor>& advisor) {
    if (!ensureAdvisor(advisor)) return;
    Analytics analytics;
    std::string summary = analytics.buildPromptSummary(expenses);
    streamQuery(*advisor, summary);
}

// 2. Custom question — user types anything
static void aiCustomQuestion(const std::vector<Expense>& expenses,
                              std::unique_ptr<AIAdvisor>& advisor) {
    if (!ensureAdvisor(advisor)) return;

    Analytics analytics;
    std::string summary = analytics.buildPromptSummary(expenses);

    std::cout << "\n  Type your question (e.g. 'How can I save on food?'):\n";
    std::cout << "  > ";
    std::string question;
    std::getline(std::cin, question);
    if (question.empty()) {
        std::cout << "  [!] No question entered.\n";
        return;
    }

    // Combine expense context with the user's question
    std::string prompt = "My current expenses:\n" + summary +
                         "\n\nQuestion: " + question;
    streamQuery(*advisor, prompt);
}

// 3. Category-specific advice
static void aiCategoryAdvice(const std::vector<Expense>& expenses,
                              std::unique_ptr<AIAdvisor>& advisor) {
    if (!ensureAdvisor(advisor)) return;

    std::cout << "\n  Enter category name (e.g. Food, Transport, Utilities): ";
    std::string category;
    std::getline(std::cin, category);
    if (category.empty()) {
        std::cout << "  [!] No category entered.\n";
        return;
    }

    // Filter expenses for this category
    std::string catLower = category;
    std::transform(catLower.begin(), catLower.end(), catLower.begin(), ::tolower);

    std::ostringstream catSummary;
    double total = 0.0;
    int    count = 0;
    for (const auto& e : expenses) {
        std::string eLower = e.categoryStr();
        std::transform(eLower.begin(), eLower.end(), eLower.begin(), ::tolower);
        if (eLower.find(catLower) != std::string::npos) {
            catSummary << "  - " << e.description
                       << ": ₹" << std::fixed << std::setprecision(2) << e.amount
                       << " (" << e.date << ")\n";
            total += e.amount;
            ++count;
        }
    }

    if (count == 0) {
        std::cout << "  [!] No expenses found for category: " << category << "\n";
        return;
    }

    std::string prompt = "My " + category + " expenses (" + std::to_string(count) +
                         " items, total ₹" + std::to_string((int)total) + "):\n" +
                         catSummary.str() +
                         "\nGive 3 specific tips to reduce my " + category + " spending.";
    streamQuery(*advisor, prompt);
}

// 4. Month-over-month comparison
static void aiMonthlyComparison(const std::vector<Expense>& expenses,
                                 std::unique_ptr<AIAdvisor>& advisor) {
    if (!ensureAdvisor(advisor)) return;

    Analytics   analytics;
    std::string thisMonth = currentMonth();
    std::string lastMonth = previousMonth();

    auto thisFiltered = analytics.filterByMonth(expenses, thisMonth);
    auto lastFiltered = analytics.filterByMonth(expenses, lastMonth);

    if (thisFiltered.empty() && lastFiltered.empty()) {
        std::cout << "  [!] Not enough data for comparison (need at least one month).\n";
        return;
    }

    // Build a simple totals summary for each month
    auto buildMonthSummary = [](const std::vector<Expense>& list, const std::string& label) {
        double total = 0.0;
        std::map<std::string, double> byCategory;
        for (const auto& e : list) {
            total += e.amount;
            byCategory[e.categoryStr()] += e.amount;
        }
        std::ostringstream ss;
        ss << label << " (total ₹" << std::fixed << std::setprecision(2) << total << "):\n";
        for (const auto& kv : byCategory)
            ss << "  - " << kv.first << ": ₹" << std::fixed << std::setprecision(2) << kv.second << "\n";
        return ss.str();
    };

    std::string prompt =
        buildMonthSummary(lastFiltered, lastMonth) + "\n" +
        buildMonthSummary(thisFiltered, thisMonth) + "\n" +
        "Compare these two months. What changed? Give 3 tips based on the trend.";

    streamQuery(*advisor, prompt);
}

// ── Main AI handler ───────────────────────────────────────────────────────────

static void handleAI(const std::vector<Expense>& expenses,
                      std::unique_ptr<AIAdvisor>& advisor) {
    while (true) {
        std::cout << "\n  ─── AI Financial Advisor ──────────────────────\n\n";
        printAIMenu();

        std::string choice;
        std::getline(std::cin, choice);

        if      (choice == "1") { aiGeneralTips(expenses, advisor);       pause(); }
        else if (choice == "2") { aiCustomQuestion(expenses, advisor);    pause(); }
        else if (choice == "3") { aiCategoryAdvice(expenses, advisor);    pause(); }
        else if (choice == "4") { aiMonthlyComparison(expenses, advisor); pause(); }
        else if (choice == "0") { break; }
        else {
            std::cout << "  [!] Invalid option.\n";
            pause();
        }
    }
}

static void handleExport(const std::vector<Expense>& expenses,
                          std::unique_ptr<AIAdvisor>& advisor) {
    Analytics analytics;
    std::string ts       = currentTimestamp();
    std::string filePath = "reports/report_" + ts + ".txt";

    std::string aiInsight;
    if (advisor && advisor->isLoaded()) {
        std::cout << "  [~] Generating AI insight for report...\n";
        std::string summary = analytics.buildPromptSummary(expenses);
        aiInsight = advisor->query(summary, nullptr);
    }

    analytics.exportReport(expenses, filePath, aiInsight);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    const std::string CSV_PATH = "data/expenses.csv";

    Storage              storage(CSV_PATH);
    std::vector<Expense> expenses;
    StorageError         loadErr = storage.load(expenses);

    if (loadErr.failed) {
        std::cerr << "  [!] " << loadErr.message << "\n";
        return 1;
    }

    std::unique_ptr<AIAdvisor> advisor;

    printBanner();
    std::cout << "  [✓] " << expenses.size() << " expense(s) loaded from " << CSV_PATH << "\n\n";

    while (true) {
        printMenu();

        std::string choice;
        std::getline(std::cin, choice);

        clearScreen();
        printBanner();

        if      (choice == "1") { handleAdd(expenses, storage);         pause(); }
        else if (choice == "2") { handleViewAll(expenses);               pause(); }
        else if (choice == "3") { handleViewMonth(expenses);             pause(); }
        else if (choice == "4") { handleAnalytics(expenses);             pause(); }
        else if (choice == "5") { handleDelete(expenses, storage);       pause(); }
        else if (choice == "6") { handleAI(expenses, advisor);                    }
        else if (choice == "7") { handleExport(expenses, advisor);       pause(); }
        else if (choice == "0") {
            std::cout << "  Goodbye!\n\n";
            break;
        } else {
            std::cout << "  [!] Invalid option. Enter 0–7.\n";
            pause();
        }
    }

    return 0;
}
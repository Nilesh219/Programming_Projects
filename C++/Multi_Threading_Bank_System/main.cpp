#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <ctime>

using namespace std;

// Mutex for thread-safe operations
std::mutex globalMutex;

// Transaction class
class Transaction {
public:
    int transactionId;
    string type;
    int amount;
    int fromAccount;
    int toAccount;
    string timestamp;

    Transaction(int id, string typ, int amt, int from, int to) 
        : transactionId(id), type(typ), amount(amt), fromAccount(from), toAccount(to) {
        time_t now = time(0);
        timestamp = ctime(&now);
        timestamp.pop_back(); // Remove newline
    }

    void display() const {
        lock_guard<mutex> guard(globalMutex);
        cout << "\nTransaction ID: " << transactionId
             << "\nType: " << type
             << "\nAmount: " << amount
             << "\nFrom Account: " << fromAccount
             << "\nTo Account: " << (toAccount == -1 ? "N/A" : to_string(toAccount))
             << "\nTime: " << timestamp << endl;
    }
};

// Account class
class Account {
private:
    int accountNumber;
    string accountName;
    int balance;
    string accountType;
    mutable mutex accMutex; // Ensure thread-safety for operations

public:
    Account(int accNo, string name, int bal, string type)
        : accountNumber(accNo), accountName(name), balance(bal), accountType(type) {}

    // Delete copy constructor to prevent mutex-related copy issues
    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

    // Allow move constructor
    Account(Account&& other) noexcept 
        : accountNumber(other.accountNumber), accountName(move(other.accountName)), 
          balance(other.balance), accountType(move(other.accountType)) {}

    int getAccountNumber() const { return accountNumber; }

    void display() const {
        lock_guard<mutex> guard(accMutex);
        cout << "\nAccount Number: " << accountNumber 
             << "\nAccount Name: " << accountName 
             << "\nBalance: " << balance 
             << "\nAccount Type: " << accountType << endl;
    }

    bool deposit(int amount) {
        if (amount <= 0) return false;
        lock_guard<mutex> guard(accMutex);
        balance += amount;
        return true;
    }

    bool withdraw(int amount) {
        lock_guard<mutex> guard(accMutex);
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        return true;
    }

    bool transfer(Account& toAccount, int amount) {
        lock_guard<mutex> guard(accMutex);
        if (amount <= 0 || amount > balance) return false;
        
        balance -= amount;
        lock_guard<mutex> targetGuard(toAccount.accMutex);
        toAccount.balance += amount;
        return true;
    }
};

// Bank class
class Bank {
private:
    vector<unique_ptr<Account>> accounts;
    vector<Transaction> transactions;
    int nextAccountNumber = 1000;

public:
    Bank() {
        cout << "Welcome to the Bank!" << endl;
    }

    int createAccount(string name, string type, int initialBalance) {
        lock_guard<mutex> guard(globalMutex);
        accounts.push_back(make_unique<Account>(nextAccountNumber, name, initialBalance, type));
        return nextAccountNumber++;
    }

    Account* findAccount(int accountNumber) {
        for (auto& acc : accounts) {
            if (acc->getAccountNumber() == accountNumber) {
                return acc.get();
            }
        }
        return nullptr;
    }

    void displayAllAccounts() {
        for (const auto& acc : accounts) {
            acc->display();
        }
    }

    void recordTransaction(string type, int amount, int fromAccount, int toAccount = -1) {
        lock_guard<mutex> guard(globalMutex);
        transactions.emplace_back(transactions.size() + 1, type, amount, fromAccount, toAccount);
    }

    void displayTransactions() {
        cout << "\nTransaction History:\n";
        for (const auto& trans : transactions) {
            trans.display();
        }
    }
};

// User interaction function
void userMenu(Bank& bank) {
    while (true) {
        int choice;
        cout << "\n1. Create Account\n2. Deposit\n3. Withdraw\n4. Transfer\n5. View Account\n6. View Transactions\n7. Exit\nChoose an option: ";
        cin >> choice;

        if (choice == 7) break;

        string name, type;
        int accNum, amount, toAcc;
        switch (choice) {
            case 1:
                cout << "Enter Name: "; cin >> name;
                cout << "Enter Account Type (Savings/Current): "; cin >> type;
                cout << "Enter Initial Balance: "; cin >> amount;
                accNum = bank.createAccount(name, type, amount);
                cout << "Account Created! Your Account Number is: " << accNum << endl;
                break;
            case 2:
                cout << "Enter Account Number: "; cin >> accNum;
                cout << "Enter Deposit Amount: "; cin >> amount;
                if (Account* acc = bank.findAccount(accNum)) {
                    if (acc->deposit(amount)) {
                        cout << "Deposited Successfully!" << endl;
                        bank.recordTransaction("Deposit", amount, accNum);
                    }
                } else cout << "Invalid Account!" << endl;
                break;
            case 3:
                cout << "Enter Account Number: "; cin >> accNum;
                cout << "Enter Withdrawal Amount: "; cin >> amount;
                if (Account* acc = bank.findAccount(accNum)) {
                    if (acc->withdraw(amount)) {
                        cout << "Withdrawal Successful!" << endl;
                        bank.recordTransaction("Withdraw", amount, accNum);
                    }
                } else cout << "Invalid Account!" << endl;
                break;
            case 4:
                cout << "Enter From Account Number: "; cin >> accNum;
                cout << "Enter To Account Number: "; cin >> toAcc;
                cout << "Enter Transfer Amount: "; cin >> amount;
                if (Account* from = bank.findAccount(accNum)) {
                    if (Account* to = bank.findAccount(toAcc)) {
                        if (from->transfer(*to, amount)) {
                            cout << "Transfer Successful!" << endl;
                            bank.recordTransaction("Transfer", amount, accNum, toAcc);
                        }
                    }
                } else cout << "Invalid Accounts!" << endl;
                break;
            case 5: 
                cout << "Enter Account Number to View (Enter 0 for All Accounts): ";
                cin >> accNum;
                if (accNum == 0) {
                    bank.displayAllAccounts();
                } else {
                    Account* acc = bank.findAccount(accNum);
                    if (acc) {
                        acc->display();
                    } else {
                        cout << "Account not found!" << endl;
                    }
                }
                break;
            case 6:
                bank.displayTransactions();
                break;
            default:
                cout << "Invalid Choice!" << endl;
        }
    }
}

// Main Function
int main() {
    Bank myBank;
    userMenu(myBank);
    return 0;
}
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/Value.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "helper.hpp"
#include "lexer.hpp"

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::IRBuilder<llvm::NoFolder>> Builder;

static void InitializeModule() {
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule  = std::make_unique<llvm::Module>("MyModule", *TheContext);
    Builder    = std::make_unique<llvm::IRBuilder<llvm::NoFolder>>(*TheContext);
}

int symbol;

YYLVAL yylval;

void next_symbol() { symbol = yylex(); }

#define ERROR(msg, val)                                            \
    std::cerr << "(line " << __LINE__ << ") " << msg << " " << val \
              << "(char: " << (char)val << ")" << std::endl;

#define SYMBOL_ERROR ERROR("Unknown symbol:", symbol);

#define ASSERT_SYMBOL(s)              \
    if (symbol != s) {                \
        SYMBOL_ERROR;                 \
        ERROR("Expected symbol:", s); \
        std::exit(EXIT_FAILURE);      \
    }

//===----------------------------------------------------------------------===//
// AST NODES
//===----------------------------------------------------------------------===//
class GenericASTNode {
  public:
    virtual ~GenericASTNode()      = default;
    virtual std::string toString() = 0;
    virtual llvm::Value *codegen() = 0;
};

using ASTNode = std::unique_ptr<GenericASTNode>;

class NumberASTNode : public GenericASTNode {
    int Val;

  public:
    NumberASTNode(int Val) { this->Val = Val; }

    std::string toString() { return std::to_string(Val); }

    llvm::Value *codegen() {
        return llvm::ConstantInt::get(*TheContext,
                                      llvm::APInt(32, this->Val, true));
    }
};

class BinaryExprAST : public GenericASTNode {
    char Op;
    ASTNode LHS, RHS;

  public:
    BinaryExprAST(char Op, ASTNode LHS, ASTNode RHS) {
        this->Op  = Op;
        this->LHS = std::move(LHS);
        this->RHS = std::move(RHS);
    }

    std::string toString() {
        return "(" + this->LHS->toString() + " " + Op + " " +
               this->RHS->toString() + ")";
    }

    llvm::Value *codegen() {
        switch (this->Op) {
            case '+':
                return Builder->CreateAdd(this->LHS->codegen(), this->RHS->codegen(),
                                          "addtmp");
            case '-':
                return Builder->CreateSub(this->LHS->codegen(), this->RHS->codegen(),
                                          "subtmp");
            case '*':
                return Builder->CreateMul(this->LHS->codegen(), this->RHS->codegen(),
                                          "multmp");
            case '/':
                return Builder->CreateSDiv(this->LHS->codegen(), this->RHS->codegen(),
                                           "divtmp");
            case '%':
                return Builder->CreateSRem(this->LHS->codegen(), this->RHS->codegen(),
                                           "modtmp");

            default:
                break;
        }

        ERROR("Unknown binary operator:", this->Op);
        std::exit(EXIT_FAILURE);
    }
};

using namespace llvm;

class IfStatementAST : public GenericASTNode {
    ASTNode Cond, TrueExpr, FalseExpr;

  public:
    IfStatementAST(ASTNode Cond, ASTNode TrueExpr, ASTNode FalseExpr) {
        this->Cond     = std::move(Cond);
        this->TrueExpr = std::move(TrueExpr);

        if (FalseExpr == nullptr)
            this->FalseExpr = std::make_unique<NumberASTNode>(0);
        else
            this->FalseExpr = std::move(FalseExpr);
    }

    std::string toString() {
        std::ostringstream oss;

        oss << "IF " << this->Cond->toString() << " THEN "
            << this->TrueExpr->toString() << " ELSE " << this->FalseExpr->toString()
            << " END";

        return oss.str();
    }

    llvm::Value *codegen() {
        llvm::Value *condition = this->Cond->codegen();

        if (condition == nullptr)
            return nullptr;

        llvm::Value *zeroValue =
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
        llvm::Value *comparison =
            Builder->CreateICmpNE(condition, zeroValue, "cond");

        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

        llvm::BasicBlock *trueBlock =
            llvm::BasicBlock::Create(*TheContext, "trueBlock", TheFunction);
        llvm::BasicBlock *falseBlock =
            llvm::BasicBlock::Create(*TheContext, "falseBlock", TheFunction);
        llvm::BasicBlock *mergeBlock =
            llvm::BasicBlock::Create(*TheContext, "mergeBlock", TheFunction);

        Builder->CreateCondBr(comparison, trueBlock, falseBlock);

        Builder->SetInsertPoint(trueBlock);
        Builder->CreateBr(mergeBlock);

        Builder->SetInsertPoint(falseBlock);
        Builder->CreateBr(mergeBlock);

        Builder->SetInsertPoint(mergeBlock);

        PHINode *PN =
            Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, "PHItmp");

        llvm::Value *trueExpr = this->TrueExpr->codegen();
        PN->addIncoming(trueExpr, trueBlock);

        llvm::Value *falseExpr = this->FalseExpr->codegen();
        PN->addIncoming(falseExpr, falseBlock);

        return PN;
    }
};

class WhileStatementAST : public GenericASTNode {
    ASTNode Cond, Body;

  public:
    WhileStatementAST(ASTNode Cond, ASTNode Body) {
        this->Cond = std::move(Cond);
        this->Body = std::move(Body);
    }

    std::string toString() {
        std::ostringstream oss;

        oss << "WHILE " << this->Cond->toString() << " THEN "
            << this->Body->toString();

        return oss.str();
    }

    llvm::Value *codegen() {
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

        llvm::BasicBlock *condBlock =
            llvm::BasicBlock::Create(*TheContext, "condBlock", TheFunction);
        llvm::BasicBlock *bodyBlock =
            llvm::BasicBlock::Create(*TheContext, "bodyBlock", TheFunction);
        llvm::BasicBlock *endBlock =
            llvm::BasicBlock::Create(*TheContext, "endBlock", TheFunction);

        Builder->CreateBr(condBlock);

        Builder->SetInsertPoint(condBlock);

        llvm::Value *condition = this->Cond->codegen();
        llvm::Value *zeroValue =
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
        llvm::Value *comparison =
            Builder->CreateICmpNE(condition, zeroValue, "cond");

        Builder->CreateCondBr(comparison, bodyBlock, endBlock);

        Builder->SetInsertPoint(bodyBlock);
        this->Body->codegen();
        Builder->CreateBr(condBlock);

        Builder->SetInsertPoint(endBlock);

        return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    }
};

class DoWhileStatementAST : public GenericASTNode {
    ASTNode Cond, Body;

  public:
    DoWhileStatementAST(ASTNode Cond, ASTNode Body) {
        this->Cond = std::move(Cond);
        this->Body = std::move(Body);
    }

    std::string toString() {
        std::ostringstream oss;

        oss << "DO " << this->Body->toString() << " WHILE "
            << this->Cond->toString();

        return oss.str();
    }

    llvm::Value *codegen() {
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

        llvm::BasicBlock *bodyBlock =
            llvm::BasicBlock::Create(*TheContext, "bodyBlock", TheFunction);
        llvm::BasicBlock *condBlock =
            llvm::BasicBlock::Create(*TheContext, "condBlock", TheFunction);
        llvm::BasicBlock *endBlock =
            llvm::BasicBlock::Create(*TheContext, "endBlock", TheFunction);

        Builder->CreateBr(bodyBlock);

        Builder->SetInsertPoint(bodyBlock);
        this->Body->codegen();

        Builder->SetInsertPoint(condBlock);

        llvm::Value *condition = this->Cond->codegen();
        llvm::Value *zeroValue =
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
        llvm::Value *comparison =
            Builder->CreateICmpNE(condition, zeroValue, "cond");

        Builder->CreateCondBr(comparison, bodyBlock, endBlock);

        Builder->SetInsertPoint(endBlock);

        return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    }
};

class StatementsAST : public GenericASTNode {
    std::vector<ASTNode> Statements;

  public:
    StatementsAST() {}

    void addNode(ASTNode node) { Statements.push_back(std::move(node)); }

    std::string toString() {
        std::ostringstream oss;

        oss << "{ ";

        for (auto &Statement : this->Statements)
            oss << Statement->toString() << "; ";

        oss << "}";

        return oss.str();
    }

    llvm::Value *codegen() {
        for (auto &Statement : this->Statements)
            Statement->codegen();

        return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    }
};

std::map<char, llvm::AllocaInst *> allocatedVariables;

class VariableDeclarationASTNode : public GenericASTNode {
    char Name;

  public:
    VariableDeclarationASTNode(char Name) : Name(Name) {}

    std::string toString() {
        std::ostringstream oss;
        oss << "var " << Name;
        return oss.str();
    }

    llvm::Value *codegen() {
        AllocaInst *ptr =
            Builder->CreateAlloca(Type::getInt32Ty(*TheContext), nullptr, "myVar");

        allocatedVariables[Name] = ptr;

        return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    }
};

class VariableReadASTNode : public GenericASTNode {
    char Name;

  public:
    VariableReadASTNode(char Name) : Name(Name) {}

    std::string toString() {
        std::ostringstream oss;
        oss << Name;
        return oss.str();
    }

    llvm::Value *
    codegen() {
        llvm::AllocaInst *ptr = allocatedVariables[Name];

        if (ptr == nullptr)
            return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));

        return Builder->CreateLoad(ptr->getAllocatedType(), ptr, "myVar");
    }
};

class VariableAssignASTNode : public GenericASTNode {
    char Name;
    ASTNode Value;

  public:
    VariableAssignASTNode(char Name, ASTNode Value) {
        this->Name  = Name;
        this->Value = std::move(Value);
    }
    std::string toString() {
        std::ostringstream oss;
        oss << "assign " << Name << " = " << Value->toString();
        return oss.str();
    }

    llvm::Value *
    codegen() {
        llvm::AllocaInst *ptr = allocatedVariables[Name];

        if (ptr == nullptr)
            return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));

        llvm::Value *ToAssign = Value->codegen();
        Builder->CreateStore(ToAssign, ptr);

        return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    }
};

//===----------------------------------------------------------------------===//
// CODE GEN
//===----------------------------------------------------------------------===//
void CodeGenTopLevel(ASTNode AST_Root) {
    std::cout << "Generating code for: " << AST_Root->toString() << std::endl;

    // Create an anonymous function with no parameters
    std::vector<llvm::Type *> ArgumentsTypes(0);

    /*FunctionType* FT = FunctionType::get(Type::getInt32Ty(*TheContext),
     * ArgumentsTypes, false);*/
    llvm::FunctionType *FT = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*TheContext), ArgumentsTypes, false);

    llvm::Function *F = llvm::Function::Create(
        FT, llvm::Function::ExternalLinkage, "main", TheModule.get());

    // Create a label 'entry' and set it to the current position in the builder
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    // Generate the code for the body of the function and return the result
    if (llvm::Value *RetVal = AST_Root->codegen()) {
        Builder->CreateRet(RetVal);
    }

    auto Filename = "output.ll";
    std::error_code EC;
    llvm::raw_fd_ostream dest(Filename, EC);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
        return;
    }

    F->print(llvm::errs());
    F->print(dest);
    F->eraseFromParent();
}

//===----------------------------------------------------------------------===//
// PARSER
//===----------------------------------------------------------------------===//

// Z ::= STATEMENTS
ASTNode Z();

// STATEMENTS ::= STATEMENT (';' STATEMENTS)*
ASTNode STATEMENTS();

// STATEMENT ::= E_AS | E_IF | E_WHILE | E_DO_WHILE | VAR_DECL | VAR_ASSIGN
ASTNode STATEMENT();

// VAR_DECL ::= var identifier.
ASTNode VAR_DECL();

// VAR_ASSIGN ::= assign identifier '=' E_AS.
ASTNode VAR_ASSIGN();

// E_IF ::= if '(' E_AS ')' '{' STATEMENTS '}' else '{' STATEMENTS '}'.
ASTNode E_IF();

// E_WHILE ::= while '(' STATEMENTS ')' '{' E_AS '}'.
ASTNode E_WHILE();

// E_DO_WHILE ::= do '{' STATEMENTS '}' while '(' E_AS ')'.
ASTNode E_DO_WHILE();

// E_AS ::= E_MDR ('+' | '-' E_MDR)*.
ASTNode E_AS();

// E_MDR ::= T ('*' | '/' | '%' T)*.
ASTNode E_MDR();

// T ::= i | '(' E_AS ')' | identifier.
ASTNode T();

ASTNode Z() { return STATEMENTS(); }

ASTNode STATEMENTS() {
    std::unique_ptr<StatementsAST> statements = std::make_unique<StatementsAST>();

    auto node = STATEMENT();
    statements->addNode(std::move(node));

    while (symbol == ';') {
        next_symbol();
        auto next = STATEMENT();
        statements->addNode(std::move(next));
    }

    return statements;
}

ASTNode STATEMENT() {
    if (symbol == VAR)
        return VAR_DECL();

    if (symbol == ASSIGN)
        return VAR_ASSIGN();

    if (symbol == IF)
        return E_IF();

    if (symbol == WHILE)
        return E_WHILE();

    if (symbol == DO)
        return E_DO_WHILE();

    return E_AS();
}

ASTNode VAR_DECL() {
    ASSERT_SYMBOL(VAR);
    next_symbol();

    ASSERT_SYMBOL(IDENTIFIER);

    char value = yylval.cVal;
    next_symbol();
    return std::make_unique<VariableDeclarationASTNode>(value);
}

ASTNode VAR_ASSIGN() {
    ASSERT_SYMBOL(ASSIGN);
    next_symbol();

    ASSERT_SYMBOL(IDENTIFIER);

    char value = yylval.cVal;
    next_symbol();

    ASSERT_SYMBOL('=');
    next_symbol();

    ASTNode expr = E_AS();

    return std::make_unique<VariableAssignASTNode>(value, std::move(expr));
}

ASTNode E_IF() {
    ASSERT_SYMBOL(IF);
    next_symbol();

    ASSERT_SYMBOL('(');
    next_symbol();

    ASTNode cond = E_AS();

    ASSERT_SYMBOL(')');
    next_symbol();

    ASSERT_SYMBOL('{');
    next_symbol();

    ASTNode trueExpr = STATEMENTS();

    ASSERT_SYMBOL('}');
    next_symbol();

    if (symbol != ELSE)
        return std::make_unique<IfStatementAST>(std::move(cond),
                                                std::move(trueExpr), nullptr);
    next_symbol();

    ASSERT_SYMBOL('{');
    next_symbol();

    ASTNode falseExpr = STATEMENTS();

    ASSERT_SYMBOL('}');
    next_symbol();

    return std::make_unique<IfStatementAST>(std::move(cond), std::move(trueExpr),
                                            std::move(falseExpr));
}

ASTNode E_WHILE() {
    ASSERT_SYMBOL(WHILE);
    next_symbol();

    ASSERT_SYMBOL('(');
    next_symbol();

    ASTNode cond = E_AS();

    ASSERT_SYMBOL(')');
    next_symbol();

    ASSERT_SYMBOL('{');
    next_symbol();

    ASTNode body = STATEMENTS();

    ASSERT_SYMBOL('}');
    next_symbol();

    return std::make_unique<WhileStatementAST>(std::move(cond), std::move(body));
}

ASTNode E_DO_WHILE() {
    ASSERT_SYMBOL(DO);
    next_symbol();

    ASSERT_SYMBOL('{');
    next_symbol();

    ASTNode body = STATEMENTS();

    ASSERT_SYMBOL('}');
    next_symbol();

    ASSERT_SYMBOL(WHILE);
    next_symbol();

    ASSERT_SYMBOL('(');
    next_symbol();

    ASTNode cond = E_AS();

    ASSERT_SYMBOL(')');
    next_symbol();

    return std::make_unique<DoWhileStatementAST>(std::move(cond),
                                                 std::move(body));
}

ASTNode E_AS() {
    ASTNode acc = E_MDR();

    while (true) {
        switch (symbol) {
            case '+':
            case '-':
                break;

            default:
                return acc;
        }

        char op = symbol;
        next_symbol();
        auto op1 = E_MDR();
        acc      = std::make_unique<BinaryExprAST>(op, std::move(acc), std::move(op1));
    }
}

ASTNode E_MDR() {
    ASTNode acc = T();

    while (true) {
        switch (symbol) {
            case '*':
            case '/':
            case '%':
                break;

            default:
                return acc;
        }

        char op = symbol;
        next_symbol();

        auto op1 = T();
        acc      = std::make_unique<BinaryExprAST>(op, std::move(acc), std::move(op1));
    }
}

ASTNode T() {
    if (symbol == IDENTIFIER) {
        char value = yylval.cVal;
        next_symbol();
        return std::make_unique<VariableReadASTNode>(value);
    }

    if (symbol == NUMBER) {
        int value = yylval.iVal;
        next_symbol();
        return std::make_unique<NumberASTNode>(value);
    }

    if (symbol == '(') {
        next_symbol();
        ASTNode node = E_AS();

        ASSERT_SYMBOL(')');
        next_symbol();
        return node;
    }

    SYMBOL_ERROR;
    std::exit(EXIT_FAILURE);
}

//===----------------------------------------------------------------------===//
// MAIN FUNCTION
//===----------------------------------------------------------------------===//

int main() {
    InitializeModule();

    while (1) {
        next_symbol();

        if (symbol == '\0')
            break;

        CodeGenTopLevel(Z());
    }

    return 0;
}

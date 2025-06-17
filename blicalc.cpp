// blicalc -- Blister Calculator
// Only standart libraries
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <cmath>
#include <complex>

// Data types
using Complex = std :: complex <double>;
using OptValue = std :: optional <Complex>;

// Tokens structure
struct Token {
    enum Type {
        Number,
        Identifier,
        Operator,
        LeftParen,
        RightParen,
        Comma,
        End
    } type;
    std :: string text;
};

// Abstract syntax tree
struct ExprNode {
    enum Type {
        Value,
        BinaryOp,
        Function,
        UnaryOp
    } type;
    std :: string op;
    std :: vector <ExprNode> children;
    OptValue val;
};

// Function args
static const std :: map <std :: string, int> prec = {
    {"+", 1}, {"-", 1}, {"*", 2}, {"/", 2}, {"%", 2}, {"^", 3}, {"u+", 4}, {"u-", 4}
};

static const std :: map <std :: string, int> func_argc = {
    {"sin", 1}, {"cos", 1}, {"tan", 1}, {"cot", 1}, {"sec", 1}, {"csc", 1}, {"log", 2}, {"rt", 2}
};

// Child lexer
bool is_func (const std :: string & f) { return func_argc.count (f) > 0; }
bool is_left_assoc (const std :: string & op) { return op != "^" && op.find ('u') != 0; }
int precedence (const std :: string & op) { return prec.count (op) ? prec.at (op) : -1; }

// Lexer
std :: vector <Token> lexer (const std :: string & s) {
    std :: vector <Token> tokens; size_t i = 0;
    while (i < s.size ()) {
        if (isspace (s [i])) { i++; continue; }
        if (isdigit (s [i]) || s [i] == '.') {
            size_t j = i;
            while (j < s.size () && (isdigit (s [j]) || s [j] == '.')) j++;
            bool has_i = (j < s.size () && s [j] == 'i');
            std :: string num = s.substr (i, j - i + (has_i ? 1 : 0));
            if (has_i) j++;
            tokens.push_back ({Token :: Number, num});
            i = j; continue;
        }
        if (isalpha (s [i])) {
            size_t j = i;
            while (j < s.size () && isalpha (s [j])) j++;
            tokens.push_back ({Token :: Identifier, s.substr (i, j-i)});
            i = j; continue;
        }
        char c = s [i++];
        switch (c) {
            case '+': case '-': case '*': case '/': case '%': case '^': tokens.push_back ({Token :: Operator, std :: string (1, c)}); break;
            case '(': tokens.push_back ({Token :: LeftParen, "("}); break;
            case ')': tokens.push_back ({Token :: RightParen, ")"}); break;
            case ',': tokens.push_back ({Token :: Comma, ","}); break;
            default: break;
        }
    }
    tokens.push_back ({Token :: End, ""});
    return tokens;
}

// Constants
OptValue parse_const (const std :: string & id) {
    if (id == "pi") return Complex (M_PI, 0);
    if (id == "e") return Complex (M_E, 0);
    return std :: nullopt;
}

// Binary operations
OptValue bin_op (const std :: string & op, const Complex & l, const Complex & r) {
    if (op == "+") return l + r;
    if (op == "-") return l - r;
    if (op == "*") return l * r;
    if (op == "/") return (r == Complex (0,0)) ? std :: nullopt : OptValue (l / r);
    if (op == "%") return std :: nullopt;
    if (op == "^") return std :: pow(l,r);
    return std::nullopt;
}

// Built-in functions
OptValue call_func (const std :: string & f, const std :: vector <Complex> & args) {
    if (f == "sin") return std :: sin (args [0]);
    if (f == "cos") return std :: cos (args [0]);
    if (f == "tan") return std :: tan (args [0]);
    if (f == "cot") return 1.0 / std :: tan (args [0]);
    if (f == "sec") return 1.0 / std :: cos (args [0]);
    if (f == "csc") return 1.0 / std :: sin (args [0]);
    if (f == "log" && args.size () == 2) return std :: log (args [1]) / std :: log (args [0]);
    if (f == "rt" && args.size () == 2) return std :: pow (args [1], 1.0 / args [0]);
    return std :: nullopt;
}

// Parser
std :: optional <ExprNode> parse_expr (const std :: vector <Token> & tokens) {
    std :: vector <ExprNode> output;
    std :: vector <Token> ops;
    bool expect_operand = true;

    auto pop_op = [&] () -> bool {
        if (ops.empty ()) return false;
        Token top = ops.back (); ops.pop_back ();
        if (is_func (top.text)) {
            int argc = func_argc.at (top.text);
            if ((int) output.size () < argc) return false;
            std :: vector <ExprNode> args (argc);
            for (int i = argc - 1; i >= 0; i--) {
                args [i] = output.back ();
                output.pop_back ();
            }
            output.push_back ({ExprNode :: Function, top.text, args, std :: nullopt});
        } else if (top.text == "u-") {
            if (output.empty ()) return false;
            ExprNode operand = output.back (); output.pop_back ();
            output.push_back ({ExprNode :: UnaryOp, "-", {operand}, std :: nullopt});
        } else if (top.text == "u+") {
            if (output.empty ()) return false;
            ExprNode operand = output.back (); output.pop_back ();
            output.push_back ({ExprNode :: UnaryOp, "+", {operand}, std :: nullopt});
        } else {
            if (output.size () < 2) return false;
            ExprNode r = output.back (); output.pop_back ();
            ExprNode l = output.back (); output.pop_back ();
            output.push_back ({ExprNode :: BinaryOp, top.text, {l, r}, std :: nullopt});
        }
        return true;
    };

    size_t i = 0;
    while (i < tokens.size ()) {
        const Token & t = tokens [i];
        if (t.type == Token :: Number) {
            std :: string val = t.text;
            if (val.back () == 'i') {
                double im = std :: stod (val.substr (0, val.size () - 1));
                output.push_back ({ExprNode :: Value, "", {}, Complex (0, im)});
            } else {
                output.push_back ({ExprNode :: Value, "", {}, Complex (std :: stod (val), 0)});
            }
            expect_operand = false;
        } else if (t.type == Token :: Identifier) {
            auto c = parse_const (t.text);
            if (c) {
                output.push_back ({ExprNode :: Value, "", {}, *c});
                expect_operand = false;
            } else if (is_func (t.text)) {
                ops.push_back (t);
                expect_operand = true;
            } else return std :: nullopt;
        } else if (t.type == Token :: Operator) {
            std :: string op_text = t.text;
            if (expect_operand && (op_text == "-" || op_text == "+")) {
                op_text = "u" + op_text;
            }
            while (!ops.empty ()) {
                Token top = ops.back ();
                if (top.type == Token :: Operator &&
                    ((is_left_assoc (op_text) && precedence (op_text) <= precedence (top.text)) ||
                     (!is_left_assoc (op_text) && precedence (op_text) < precedence (top.text)))) {
                    if (!pop_op ()) return std :: nullopt;
                } else break;
            }
            ops.push_back ({Token :: Operator, op_text});
            expect_operand = true;
        } else if (t.type == Token :: LeftParen) {
            ops.push_back (t);
            expect_operand = true;
        } else if (t.type == Token :: RightParen) {
            bool found = false;
            while (!ops.empty ()) {
                Token top = ops.back ();
                if (top.type == Token :: LeftParen) {
                    ops.pop_back ();
                    found = true;
                    break;
                }
                if (!pop_op ()) return std :: nullopt;
            }
            if (!found) return std :: nullopt;
            if (!ops.empty () && ops.back ().type == Token :: Identifier && is_func (ops.back ().text)) {
                if (!pop_op ()) return std :: nullopt;
            }
            expect_operand = false;
        } else if (t.type == Token :: Comma) {
            bool found = false;
            while (!ops.empty ()) {
                Token top = ops.back ();
                if (top.type == Token :: LeftParen) { found = true; break; }
                if (!pop_op ()) return std :: nullopt;
            }
            if (!found) return std :: nullopt;
            expect_operand = true;
        } else if (t.type == Token :: End) break;
        i++;
    }
    while (!ops.empty ()) {
        if (ops.back ().type == Token :: LeftParen || ops.back ().type == Token :: RightParen) return std :: nullopt;
        if (!pop_op ()) return std :: nullopt;
    }
    if (output.size () != 1) return std :: nullopt;
    return output [0];
}

// Evaluation
OptValue eval_expr (const ExprNode & node) {
    switch (node.type) {
        case ExprNode :: Value: return node.val;
        case ExprNode :: BinaryOp: {
            auto l = eval_expr (node.children [0]);
            auto r = eval_expr (node.children [1]);
            if (!l || !r) return std :: nullopt;
            return bin_op (node.op, *l, *r);
        }
        case ExprNode :: Function: {
            std :: vector <Complex> args;
            for (auto & c : node.children) {
                auto v = eval_expr (c);
                if (!v) return std :: nullopt;
                args.push_back (*v);
            }
            return call_func (node.op, args);
        }
        case ExprNode :: UnaryOp: {
            auto v = eval_expr (node.children [0]);
            if (!v) return std :: nullopt;
            if (node.op == "-") return -*v;
            if (node.op == "+") return *v;
            return std :: nullopt;
        }
    }
    return std :: nullopt;
}

// Value formatting
std :: string value_to_string (const Complex & c) {
    if (c.imag () == 0) return std :: to_string (c.real ());
    return "(" + std :: to_string (c.real ()) + "+" + std :: to_string (c.imag ()) + "i)";
}

// Interpreter
int main () {
    std :: string line;
    while (true) {
        std :: cout << "> ";
        if (!std :: getline (std :: cin, line)) break;
        if (line.empty ()) continue;
        auto tokens = lexer (line);
        auto expr = parse_expr (tokens);
        if (!expr) {
            std :: cout << "Error: parse failed\n";
            continue;
        }
        auto val = eval_expr (*expr);
        if (!val) {
            std :: cout << "Error: evaluation failed\n";
            continue;
        }
        std :: cout << value_to_string (*val) << "\n";
    }
    return 0;
}

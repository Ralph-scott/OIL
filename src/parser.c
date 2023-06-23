#include <stdlib.h>
#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "errors.h"
#include "utils.h"

AST *parse_expr(Lexer *lexer);
AST *parse_statements(Lexer *lexer);

static AST *parse_node(Lexer *lexer)
{
    Token token = lexer_next(lexer);

    if (!IS_NODE[token.type]) {
        UNEXPECTED_TOKEN(token);
    }

    AST *ast = ast_alloc();

    ast->type = AST_NODE;
    ast->node = token;

    return ast;
}

static AST *parse_brackets_or_node(Lexer *lexer)
{
    if (lexer_peek(lexer).type != TOKEN_LEFT_PAREN) {
        return parse_node(lexer);
    }

    lexer_next(lexer);

    AST *ast = parse_expr(lexer);

    const Token token = lexer_next(lexer);
    if (token.type != TOKEN_RIGHT_PAREN) {
        UNEXPECTED_TOKEN(token);
    }

    return ast;
}

static AST *parse_block(Lexer *lexer)
{
    {
        const Token token = lexer_next(lexer);
        if (token.type != TOKEN_LEFT_CURLY) {
            UNEXPECTED_TOKEN(token);
        }
    }

    AST *ast = parse_statements(lexer);

    {
        const Token token = lexer_next(lexer);
        if (token.type != TOKEN_RIGHT_CURLY) {
            UNEXPECTED_TOKEN(token);
        }
    }

    return ast;
}

static AST *parse_block_or_brackets(Lexer *lexer)
{
    if (lexer_peek(lexer).type != TOKEN_LEFT_CURLY) {
        return parse_brackets_or_node(lexer);
    }
    return parse_block(lexer);
}

static AST *parse_prefix(Lexer *lexer)
{
    Token token = lexer_peek(lexer);

    if (IS_PREFIX[token.type]) {
        AST *ast = ast_alloc();

        ast->type = AST_PREFIX;
        ast->prefix = (ASTPrefix) {
            .oper = lexer_next(lexer),
            .node = parse_prefix(lexer)
        };

        return ast;
    } else {
        return parse_block_or_brackets(lexer);
    }
}

static AST *parse_infix_DM_or_prefix(Lexer *lexer)
{
    AST *lhs = parse_prefix(lexer);

    loop {
        Token token = lexer_peek(lexer);

        if (token.type == TOKEN_OPER_MUL || token.type == TOKEN_OPER_DIV) {
            AST *ast = ast_alloc();

            ast->type = AST_INFIX;
            ast->infix = (ASTInfix) {
                .oper = lexer_next(lexer),
                .lhs = lhs,
                .rhs = parse_prefix(lexer)
            };

            lhs = ast;
        } else {
            return lhs;
        }
    }
}

static AST *parse_infix_AS_or_DM(Lexer *lexer)
{
    AST *lhs = parse_infix_DM_or_prefix(lexer);

    loop {
        Token token = lexer_peek(lexer);

        if (token.type == TOKEN_OPER_ADD || token.type == TOKEN_OPER_SUB) {
            AST *ast = ast_alloc();

            ast->type = AST_INFIX;
            ast->infix = (ASTInfix) {
                .oper = lexer_next(lexer),
                .lhs = lhs,
                .rhs = parse_infix_DM_or_prefix(lexer)
            };

            lhs = ast;
        } else {
            return lhs;
        }
    }
}

static AST *parse_infix_condition_or_AS(Lexer *lexer)
{
    AST *lhs = parse_infix_AS_or_DM(lexer);

    loop {
        Token token = lexer_peek(lexer);

        if (token.type == TOKEN_OPER_EQUALS
         || token.type == TOKEN_OPER_LT
         || token.type == TOKEN_OPER_GT
         || token.type == TOKEN_OPER_LT_OR_EQUALS
         || token.type == TOKEN_OPER_GT_OR_EQUALS
         || token.type == TOKEN_OPER_NOT_EQUALS) {
            AST *ast = ast_alloc();

            ast->type = AST_INFIX;
            ast->infix = (ASTInfix) {
                .oper = lexer_next(lexer),
                .lhs = lhs,
                .rhs = parse_infix_AS_or_DM(lexer)
            };

            lhs = ast;
        } else {
            return lhs;
        }
    }
}

static AST *parse_if_statement(Lexer *lexer)
{
    Token token = lexer_next(lexer);
    if (token.type != TOKEN_IF) {
        UNEXPECTED_TOKEN(token);
    }

    AST *ast = ast_alloc();

    ast->type = AST_IF_STATEMENT;
    ast->if_statement = (ASTIfStatement) {
        .condition   = parse_expr(lexer),
        .if_branch   = parse_block(lexer),
        .else_branch = NULL
    };

    if (lexer_peek(lexer).type == TOKEN_ELSE) {
        lexer_next(lexer);
        ast->if_statement.else_branch = parse_block(lexer);
    }

    return ast;
}

static AST *parse_while_loop(Lexer *lexer)
{
    Token token = lexer_next(lexer);
    if (token.type != TOKEN_WHILE) {
        UNEXPECTED_TOKEN(token);
    }

    AST *ast = ast_alloc();

    ast->type = AST_WHILE_LOOP;
    ast->while_loop = (ASTWhileLoop) {
        .condition = parse_expr(lexer),
        .body      = parse_block(lexer)
    };

    return ast;
}

AST *parse_expr(Lexer *lexer)
{
    if (lexer_peek(lexer).type == TOKEN_IF) {
        return parse_if_statement(lexer);
    }
    return parse_infix_condition_or_AS(lexer);
}

static AST *parse_statement(Lexer *lexer)
{
    if (lexer_peek(lexer).type == TOKEN_WHILE) {
        return parse_while_loop(lexer);
    }

    AST *lhs = parse_expr(lexer);

    Token token = lexer_peek(lexer);
    switch (token.type) {
        case TOKEN_ASSIGN: {
            AST *ast = ast_alloc();

            ast->type  = AST_INFIX;
            ast->infix = (ASTInfix) {
                .oper = lexer_next(lexer),
                .lhs  = lhs,
                .rhs  = parse_expr(lexer)
            };
            
            return ast;
        }

        case TOKEN_COLON: {
            lexer_next(lexer);
            if (lhs->type != AST_NODE || lhs->node.type != TOKEN_IDENT) {
                UNEXPECTED_TOKEN(token);
            }

            AST *type = parse_expr(lexer);

            AST *ast = ast_alloc();

            ast->type  = AST_DECLARATION;
            ast->declaration = (ASTDeclaration) {
                .name = lhs->node,
                .type = type
            };

            Token token = lexer_peek(lexer);
            if (token.type == TOKEN_ASSIGN) {
                lexer_next(lexer);
                ast->declaration.value = parse_expr(lexer);
            }
            
            return ast;
        }

        default: {
            return lhs;
        }
    }
}

AST *parse_statements(Lexer *lexer)
{
    AST *ast = ast_alloc();

    ast->type = AST_BLOCK;
    ast->block = (ASTBlock) {
        .len        = 0,
        .cap        = 256,
    };

    ast->block.statements = malloc(sizeof(AST *) * ast->block.cap);

    if (ast->block.statements == NULL) {
        ALLOCATION_ERROR();
    }

    loop {
        if (IS_END[lexer_peek(lexer).type]) {
            break;
        }

        AST *statement = parse_statement(lexer);

        ast->block.statements[ast->block.len++] = statement;

        if (ast->block.len >= ast->block.cap) {
            while (ast->block.len >= ast->block.cap) {
                ast->block.cap *= 2;
            }
            ast->block.statements = realloc(ast->block.statements, sizeof(AST *) * ast->block.cap);

            if (ast->block.statements == NULL) {
                ALLOCATION_ERROR();
            }
        }

        Token token = lexer_peek(lexer);

        if (token.type != TOKEN_SEMICOLON) {
            break;
        }

        lexer_next(lexer);
    }

    return ast;
}

AST *parse(Lexer *lexer)
{
    AST *ast = parse_statements(lexer);

    Token token = lexer_peek(lexer);
    if (token.type != TOKEN_EOF) {
        UNEXPECTED_TOKEN(token);
    }

    return ast;
}

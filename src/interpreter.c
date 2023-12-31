#include <string.h>
#include "../include/interpreter.h"
#include "../include/parser.h"
#include "../include/memory.h"
#include "../include/utils/expression_eval.h"

ASTNode *evaluate(ASTNode *node, ScopeStack *memory)
{
    if (node->children_num > 0 && node->children[0]->type == NODE_EOF)
    {
        return NULL;
    }

    ASTNode *type, *name, *value, *value2, *params, *out;
    Variable *param, *var, *var2, *function;
    char *erorr_message;
    switch (node->type)
    {
    // SECTION: Variables
    case NODE_V_DEF:;
        type = evaluate(node->children[0], memory);
        name = evaluate(node->children[1], memory);
        value = evaluate(node->children[2], memory);
        out = add_variable(memory, type->value, name->value, value, NULL);
        // This validation also happens when tokenizing and parsing
        // FIXME: + Error handler and edit error message for list elements
        if (out != NULL)
        {
            printf("Incorrect variable type - tried assigning %s (%s) to incorrect value %s: %s\n", name->value, type->value, value->value, out->value);
            exit(EXIT_FAILURE);
        }
        break;
    case NODE_ASSIGN:;
        name = evaluate(node->children[0], memory);
        value = evaluate(node->children[1], memory);
        var = find_variable(memory, name->value);
        out = modify_variable_value(memory, name->value, value);
        // FIXME: + Error handler
        if (out != NULL)
        {
            printf("Incorrect variable type - tried assigning %s (%s) to incorrect value %s: %s\n", name->value, var->type, value->value, out->value);
            exit(EXIT_FAILURE);
        }
        break;
    // SECTION: Expressions and values
    case NODE_T_VAL:
    case NODE_VAL:
    case NODE_ID:
    case NODE_V_TYPE:
    case NODE_LIST_B:
        return node;
    case NODE_EXPRESSION:;
        ASTNode *postfix_epxression = infix_to_postfix(node->children, node->children_num, memory);
        value = evaluate_expression(postfix_epxression);
        if (value->type == NODE_VAL || value->type == NODE_T_VAL)
        {
            return value;
        }
        else
        {
            printf("Expression evaluation error: %s\n", value->value);
            exit(EXIT_FAILURE);
        }
        break;
    // SECTION: Functions
    case NODE_SCOPE:
        if (strcmp(node->value, "Function_Scope") == 0)
        {
            name = evaluate(node->children[0], memory);
            type = node->children[1];
            params = node->children[2];
            value = node->children[3];
            out = add_variable(memory, type->value, name->value, value, params);
            if (out != NULL)
            {
                printf("%s\n", out->value);
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(node->value, "While_Loop") == 0)
        {
            params = evaluate(node->children[0], memory);
            value = node->children[1];
            while (atoi(params->value) == 1)
            {
                evaluate(value, memory);
                params = evaluate(node->children[0], memory);
            }
        }
        else if (strcmp(node->value, "Conditional_Scope") == 0)
        {
            params = evaluate(node->children[0], memory);
            value = node->children[1];
            if (atoi(params->value) == 1)
            {
                evaluate(value, memory);
            }
            else if (node->children[2] != NULL)
            {
                value2 = node->children[2];
                evaluate(value2, memory);
            }
        }
        else
        {
            if (strcmp(node->value, "User_Scope") == 0)
            {
                push_scope(memory, 500);
            }
            for (size_t i = 0; i < node->children_num; i++)
            {
                evaluate(node->children[i], memory);
            }
            if (strcmp(node->value, "User_Scope") == 0)
            {
                pop_scope(memory);
            }
        }
        break;
    // SECTION: Keywords
    case NODE_KEYWORD:
        if (strcmp(node->value, "Print") == 0)
        {
            // FIXME: Correctly insert newlines etc.
            value = node->children[0];
            if (value->type == NODE_ID)
            {
                var = find_variable(memory, value->value);
                // FIXME: Pass all of this to error handler - return does nothing
                if (var == NULL)
                {
                    return create_ASTNode(NODE_ERROR, "Variable not found");
                }
                value = var->value;
            }
            ASTNode *result;
            switch (value->type)
            {
            case NODE_ID:;
                var = find_variable(memory, value->value);
                if (var == NULL)
                {
                    return create_ASTNode(NODE_ERROR, "Variable not found");
                }
                printf("%s\n", var->value->value);
                break;
            case NODE_VAL:
            case NODE_T_VAL:
                printf("%s\n", value->value);
                break;
            case NODE_EXPRESSION:;
                result = evaluate(value, memory);
                printf("%s\n", result->value);
                break;
            case NODE_FUNC_CALL:
                result = evaluate(value, memory);
                value2 = value->children[0];
                var = find_variable(memory, value2->value);
                if (var == NULL)
                {
                    return create_ASTNode(NODE_ERROR, "Variable not found");
                }
                printf("%s\n", var->return_value->value);
                break;
            case LIST_B:
                for (size_t i = 0; i < value->children_num; i++)
                {
                    printf("%s ", value->children[i]->value);
                }
                printf("\n");
                break;
            default:
                break;
            }
        }
        break;
    // SECTION: Function call
    case NODE_FUNC_CALL:
        name = evaluate(node->children[0], memory);
        params = node->children[1];
        function = find_variable(memory, name->value);
        if (function == NULL)
        {
            return create_ASTNode(NODE_ERROR, "Variable not found");
        }
        ASTNode *curr;
        push_scope(memory, 250);
        for (size_t i = 0; i < params->children_num; i++)
        {
            curr = params->children[i];
            if (curr->type != NODE_ID)
            {
                // FIXME: Implement bool handling and void
                switch (curr->type)
                {
                case NODE_VAL:
                    add_variable(memory, "int", function->params->children[i]->value, curr, NULL);
                    break;
                case NODE_T_VAL:
                    if (curr->value[0] == '\'')
                    {
                        add_variable(memory, "char", function->params->children[i]->value, curr, NULL);
                    }
                    else
                    {
                        add_variable(memory, "string", function->params->children[i]->value, curr, NULL);
                    }
                    break;
                default:
                    // FIXME: Make error messages formated - sprintf to erorr_message
                    return create_ASTNode(NODE_ERROR, "Incorrect function parameter passed");
                    break;
                }
            }
            else
            {
                var = find_variable(memory, curr->value);
                if (var == NULL)
                {
                    return create_ASTNode(NODE_ERROR, "Variable not found");
                }
                out = add_variable(memory, var->type, function->params->children[i]->value, var->value, NULL);
                if (out != NULL)
                {
                    printf("%s", out->value);
                    exit(EXIT_FAILURE);
                }
            }
        }
        evaluate(function->value, memory);
        pop_scope(memory);
        return function->return_value;
        break;
    case NODE_RETURN:
        // FIXME: Validate return type and value
        // Return node
        value = node->children[0];
        // Returning function
        value2 = node->children[1];
        var = find_variable(memory, value2->value);
        if (var == NULL)
        {
            return create_ASTNode(NODE_ERROR, "Variable not found");
        }
        if (value->type == NODE_EXPRESSION)
        {
            ASTNode *exp = infix_to_postfix(value->children, value->children_num, memory);
            var->return_value = evaluate_expression(exp);
        }
        else
        {
            if (value->type == NODE_ID)
            {
                var2 = find_variable(memory, value->value);
                if (var2 == NULL)
                {
                    return create_ASTNode(NODE_ERROR, "Variable not found");
                }
                var->return_value = var2->value;
            }
            else
            {
                var->return_value = value;
            }
        }
        break;
    default:
        for (size_t i = 0; i < node->children_num; i++)
        {
            evaluate(node->children[i], memory);
        }
        break;
    }
}
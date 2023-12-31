#include "../include/memory.h"
#include "../include/parser.h"

// Initialize a symbol table
void init_SymbolTable(SymbolTable *table, size_t capacity)
{
    table->entries = (Variable *)malloc(capacity * sizeof(Variable));
    table->capacity = capacity;
    table->size = 0;
}

// Initialize a scope stack
void init_ScopeStack(ScopeStack *stack, int max_size)
{
    stack->items = (SymbolTable *)malloc(max_size * sizeof(SymbolTable));
    stack->top = -1;
    stack->max_size = max_size;
}

// Push a new scope onto the scope stack
void push_scope(ScopeStack *stack, size_t table_capacity)
{
    if (stack->top >= stack->max_size - 1)
    {
        printf("Scope stack overflow\n");
        exit(EXIT_FAILURE);
    }
    init_SymbolTable(&(stack->items[++stack->top]), table_capacity);
}

// Pop the top scope from the scope stack
void pop_scope(ScopeStack *stack)
{
    if (stack->top < 0)
    {
        printf("Scope stack underflow\n");
        exit(EXIT_FAILURE);
    }
    free(stack->items[stack->top--].entries);
}

// Add a variable to the current scope's symbol table
ASTNode *add_variable(ScopeStack *stack, char *type, char *name, ASTNode *value, ASTNode *params)
{
    if (stack->top < 0)
    {
        printf("No active scope\n");
        return create_ASTNode(NODE_ERROR, "No memory scope initialized before adding variable");
    }
    SymbolTable *current_scope = &(stack->items[stack->top]);
    if (current_scope->size >= current_scope->capacity)
    {
        printf("Symbol table overflow\n");
        exit(EXIT_FAILURE);
    }
    Variable *entry = &(current_scope->entries[current_scope->size++]);
    char *value_type;
    switch (value->type)
    {
    // FIXME: Add boolean handling
    case NODE_VAL:
        value_type = "int";
        break;
    case NODE_T_VAL:
        if (value->value[0] == '\'')
        {
            value_type = "char";
        }
        else
        {
            value_type = "string";
        }
        break;
    case NODE_SCOPE:
        value_type = type;
    case NODE_LIST_B:
        for (size_t i = 0; i < value->children_num; i++)
        {
            if ((value->children[i]->type == NODE_VAL && strcmp(type, "int") != 0) ||
                (value->children[i]->type == NODE_T_VAL && value->children[i]->value[0] == '\'' && strcmp(type, "char") != 0) ||
                (value->children[i]->type == NODE_T_VAL && value->children[i]->value[0] == '\"' && strcmp(type, "string") != 0))
            {
                value_type = "invalid";
                break;
            }
            value_type = type;
        }
        break;
    default:
        value_type = "void";
        break;
    }

    if (value != NULL && type != NULL && name != NULL && strcmp(type, value_type) == 0)
    {
        entry->type = strdup(type);
        entry->name = strdup(name);
        entry->value = value;
        entry->params = params;
        return NULL;
    }
    else
    {
        return create_ASTNode(NODE_ERROR, "Invalid variable assignment cant add the variable to memory");
    }
}

// Search for a variable in the current scope and its outer scopes
Variable *find_variable(ScopeStack *stack, const char *name)
{
    for (int i = stack->top; i >= 0; --i)
    {
        SymbolTable *current_scope = &(stack->items[i]);
        for (size_t j = 0; j < current_scope->size; ++j)
        {
            if (strcmp(current_scope->entries[j].name, name) == 0)
            {
                return &current_scope->entries[j];
            }
        }
    }
    printf("Variable '%s' not found\n", name);
    return NULL;
}

ASTNode *modify_variable_value(ScopeStack *memory, char *name, ASTNode *value_node)
{
    Variable *target = find_variable(memory, name);
    char *value_type;

    switch (value_node->type)
    {
    // FIXME: Add boolean handling
    case NODE_VAL:
        value_type = "int";
        break;
    case NODE_T_VAL:
        if (value_node->value[0] == '\'')
        {
            value_type = "char";
        }
        else
        {
            value_type = "string";
        }
        break;
    default:
        break;
    }

    if (strcmp(target->type, value_type) == 0)
    {
        target->value = value_node;
        return NULL;
    }
    else
    {
        return create_ASTNode(NODE_ERROR, "Invalid variable assignment - different types");
    }
}

void print_memory(const ScopeStack *memory)
{
    if (memory == NULL)
    {
        printf("Memory is NULL.\n");
        return;
    }

    printf("Memory contents:\n");
    for (int i = 0; i <= memory->top; i++)
    {
        SymbolTable current_table = memory->items[i];

        printf("Scope %d:\n", i);
        for (size_t j = 0; j < current_table.size; ++j)
        {
            Variable current_var = current_table.entries[j];
            printf("  Variable %zu:\n", j);
            printf("    Name: %s\n", current_var.name);
            printf("    Type: %s\n", current_var.type);
            printf("    Value: %s\n", current_var.value->value);
        }
    }
}
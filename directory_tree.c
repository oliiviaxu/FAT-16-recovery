#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "directory_tree.h"

const mode_t MODE = 0777;

void init_node(node_t *node, char *name, node_type_t type) {
    if (name == NULL) {
        name = strdup("ROOT");
        assert(name != NULL);
    }
    node->name = name;
    node->type = type;
}

file_node_t *init_file_node(char *name, size_t size, uint8_t *contents) {
    file_node_t *node = malloc(sizeof(file_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, FILE_TYPE);
    node->size = size;
    node->contents = contents;
    return node;
}

directory_node_t *init_directory_node(char *name) {
    directory_node_t *node = malloc(sizeof(directory_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, DIRECTORY_TYPE);
    node->num_children = 0;
    node->children = NULL;
    return node;
}

void add_child_directory_tree(directory_node_t *dnode, node_t *child) {
    dnode->num_children++;
    dnode->children = realloc(dnode->children, sizeof(node_t *) * dnode->num_children);

    node_t *swap = child;

    for (size_t i = 0; i < dnode->num_children - 1; i++) {
        if (strcmp(swap->name, dnode->children[i]->name) < 0) {
            node_t *temp = dnode->children[i];
            dnode->children[i] = swap;
            swap = temp;
        }
    }

    dnode->children[dnode->num_children - 1] = swap;
}

void print_helper(node_t *node, size_t level) {

    for (size_t i = 0; i < 4 * level; i++) {
        printf(" ");
    }
    printf("%s\n", node->name);

    if (node->type == DIRECTORY_TYPE) {

        directory_node_t *dnode = (directory_node_t *) node;

        for (size_t i = 0; i < dnode->num_children; i++) {
            print_helper(dnode->children[i], level + 1);
        }
    }
}

void print_directory_tree(node_t *node) {
    print_helper(node, 0);
}

void create_directory_helper(node_t *node, char *path) {
    if (node->type == FILE_TYPE) {
        FILE *file = fopen(path, "w");
        file_node_t *file_node = (file_node_t *)node;
        assert(file != NULL);
        size_t write = fwrite(file_node->contents, sizeof(uint8_t), file_node->size, file);
        assert(write == file_node->size);
        assert(fclose(file) == 0);
    } else {
        assert(node->type == DIRECTORY_TYPE);
        assert(mkdir(path, MODE) == 0);
        directory_node_t *dir = (directory_node_t *)node;

        for (size_t i = 0; i < dir->num_children; i++) {
            char *new_path = malloc(sizeof(char) * (strlen(path) + 2 + strlen(dir->children[i]->name)));
            strcpy(new_path, path);
            strcat(new_path, "/");
            strcat(new_path, dir->children[i]->name);
            create_directory_helper(dir->children[i], new_path);
            free(new_path);
        }
    }
}

void create_directory_tree(node_t *node) {
    create_directory_helper(node, node->name);
}

void free_directory_tree(node_t *node) {
    if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        free(fnode->contents);
    }
    else {
        assert(node->type == DIRECTORY_TYPE);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            free_directory_tree(dnode->children[i]);
        }
        free(dnode->children);
    }
    free(node->name);
    free(node);
}
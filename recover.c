#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "directory_tree.h"
#include "fat16.h"

const size_t MASTER_BOOT_RECORD_SIZE = 0x20B;
const size_t NUM_OF_DIRECTORY_ENTRIES = 512;

void helper(FILE *disk, directory_entry_t entry, directory_node_t *node) {
    uint8_t *contents = malloc(sizeof(uint8_t) * entry.file_size);                
    assert(fread(contents, sizeof(uint8_t), entry.file_size, disk) == entry.file_size);
    // create a file_node_t and attach to the parent node
    file_node_t *fnode = init_fi le_node(get_file_name(entry), entry.file_size, contents);
    add_child_directory_tree(node, (node_t *)fnode);
}

void follow(FILE *disk, directory_node_t *node, bios_parameter_block_t bpb) {

    for (size_t i = 0; i < NUM_OF_DIRECTORY_ENTRIES; i++) {
        directory_entry_t entry;
        assert(fread(&entry, sizeof(directory_entry_t), 1, disk) == 1);
        // get current position of the stream in its file (measured in bytes from the start of the file)
        size_t pos = ftell(disk);
        
        if (entry.filename[0] == '\0') {
            break;
        }
        if (!is_hidden(entry)) {
            size_t offset = get_offset_from_cluster(entry.first_cluster, bpb);
            fseek(disk, offset, SEEK_SET);
            if (is_directory(entry)) {
                // True if directory
                directory_node_t *dnode = init_directory_node(get_file_name(entry));
                add_child_directory_tree(node, (node_t *)dnode);
                follow(disk, dnode, bpb);
            } else {
                helper(disk, entry, node);
            }
        } 
        // reset to current position 
        fseek(disk, pos, SEEK_SET);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <image filename>\n", argv[0]);
        return 1;
    }

    FILE *disk = fopen(argv[1], "r");
    if (disk == NULL) {
        fprintf(stderr, "No such image file: %s\n", argv[1]);
        return 1;
    }

    bios_parameter_block_t bpb;

    // skip over the master boot record 
    fseek(disk, MASTER_BOOT_RECORD_SIZE, SEEK_SET);
    // read the contents of the bpb 
    assert(fread(&bpb, sizeof(bios_parameter_block_t), 1, disk) == 1);
    //get the root directory location by skipping the padding and the file allocation tables directly 
    fseek(disk, get_root_directory_location(bpb), SEEK_SET);

    directory_node_t *root = init_directory_node(NULL);
    follow(disk, root, bpb);
    print_directory_tree((node_t *) root);
    create_directory_tree((node_t *) root);
    free_directory_tree((node_t *) root);

    int result = fclose(disk);
    assert(result == 0);
}
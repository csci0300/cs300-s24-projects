#include "game_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "game.h"
#include "common.h"

// Some handy dandy macros for decompression
#define E_CAP_HEX 0x45
#define E_LOW_HEX 0x65
#define G_CAP_HEX 0x47
#define G_LOW_HEX 0x67
#define S_CAP_HEX 0x53
#define S_LOW_HEX 0x73
#define W_CAP_HEX 0x57
#define W_LOW_HEX 0x77
#define DIGIT_START 0x30
#define DIGIT_END 0x39

/** Initializes the board with walls around the edge of the board.
 *
 * Modifies values pointed to by cells_p, width_p, and height_p and initializes
 * cells array to reflect this default board.
 *
 * Returns INIT_SUCCESS to indicate that it was successful.
 *
 * Arguments:
 *  - cells_p: a pointer to a memory location where a pointer to the first
 *             element in a newly initialized array of cells should be stored.
 *  - width_p: a pointer to a memory location where the newly initialized
 *             width should be stored.
 *  - height_p: a pointer to a memory location where the newly initialized
 *              height should be stored.
 */
enum board_init_status initialize_default_board(int** cells_p, size_t* width_p,
                                                size_t* height_p) {
    *width_p = 20;
    *height_p = 10;
    int* cells = malloc(20 * 10 * sizeof(int));
    *cells_p = cells;
    for (int i = 0; i < 20 * 10; i++) {
        cells[i] = PLAIN_CELL;
    }

    // Set edge cells!
    // Top and bottom edges:
    for (int i = 0; i < 20; ++i) {
        cells[i] = FLAG_WALL;
        cells[i + (20 * (10 - 1))] = FLAG_WALL;
    }
    // Left and right edges:
    for (int i = 0; i < 10; ++i) {
        cells[i * 20] = FLAG_WALL;
        cells[i * 20 + 20 - 1] = FLAG_WALL;
    }

    // Set grass cells!
    // Top and bottom edges:
    for (int i = 1; i < 19; ++i) {
        cells[i + 20] = FLAG_GRASS;
        cells[i + (20 * (9 - 1))] = FLAG_GRASS;
    }
    // Left and right edges:
    for (int i = 1; i < 9; ++i) {
        cells[i * 20 + 1] = FLAG_GRASS;
        cells[i * 20 + 19 - 1] = FLAG_GRASS;
    }

    // Add snake
    cells[20 * 2 + 2] = FLAG_SNAKE;

    return INIT_SUCCESS;
}

/** Initialize variables relevant to the game board.
 * Arguments:
 *  - cells_p: a pointer to a memory location where a pointer to the first
 *             element in a newly initialized array of cells should be stored.
 *  - width_p: a pointer to a memory location where the newly initialized
 *             width should be stored.
 *  - height_p: a pointer to a memory location where the newly initialized
 *              height should be stored.
 *  - snake_p: a pointer to your snake struct (not used until part 3!)
 *  - board_rep: a string representing the initial board. May be NULL for
 * default board.
 */
enum board_init_status initialize_game(int** cells_p, size_t* width_p,
                                       size_t* height_p, snake_t* snake_p,
                                       char* board_rep) {
    // TODO: implement!
    enum board_init_status status;
    if (board_rep != NULL) {
        status = decompress_board_str(cells_p, width_p, height_p, snake_p, board_rep);
    } else {
        status = initialize_default_board(cells_p, width_p, height_p);
    }
    pos = 20 * 2 + 2;
    direction = INPUT_RIGHT;
    g_game_over = 0;
    g_score = 0;
    if (status == INIT_SUCCESS) {
        place_food(*cells_p, *width_p, *height_p);
    }
    
    return status;
}

/* Helper function */
int* get_cell_pos(int row, int col, int* cell_array, int board_width) {
    int* correct_postion = cell_array + (row * board_width) + col;
    return correct_postion;
}

/* Helper function for filling cells */
enum board_init_status fill_cells(char cell_type, int fill_amount, int* cells) {
    for (int i = 0; i < fill_amount; i++) {
        if (cell_type == W_CAP_HEX || cell_type == W_LOW_HEX) {
            *(cells + i) = FLAG_WALL;
        } else if (cell_type == E_CAP_HEX || cell_type == E_LOW_HEX) {
            *(cells + i) = PLAIN_CELL;
        } else if (cell_type == S_CAP_HEX || cell_type == S_LOW_HEX) {
            *(cells + i) = FLAG_SNAKE;
        } else if (cell_type == G_CAP_HEX || cell_type == G_LOW_HEX) {
            *(cells + i) = FLAG_GRASS;
        } else {
            fprintf(stderr, "unable to support cell type");
            return INIT_ERR_BAD_CHAR;
        }
    }
    return INIT_SUCCESS;
}


/* Helper to check if the board is valid */
enum board_init_status check_board_validity(int* cells, size_t width,
                                            size_t height) {
    int num_snake = 0;

    for (size_t i = 0; i < width * height; i++) {
        int current_cell = cells[i];

        if (current_cell == PLAIN_CELL || current_cell & FLAG_WALL ||
            current_cell & FLAG_FOOD || current_cell & FLAG_GRASS) {
            continue;
        } else if (current_cell & FLAG_SNAKE) {
            num_snake++;
            if (num_snake > 1) {
                fprintf(stderr, "Board Invalid: Must have exactly one snake\n");
                return INIT_ERR_WRONG_SNAKE_NUM;
            }
        } else {
            fprintf(stderr, "Board Invalid: Invalid cell with contents %d\n",
                    current_cell);
            return INIT_ERR_BAD_CHAR;
        }
    }

    if (num_snake < 1) {
        fprintf(stderr, "Board Invalid: No snake\n");
        return INIT_ERR_WRONG_SNAKE_NUM;
    }

    return INIT_SUCCESS;
}


/* Helper function using strtok */
enum board_init_status set_dimensions(size_t* width_p, size_t* height_p, char* compressed) {
    if (compressed[0] != 'B') {
        return INIT_ERR_BAD_CHAR;
    }

    //get rid of B
    compressed += 1;

    //get the height
    char* rest = strtok(compressed, "x");
    *height_p = atoi(compressed);
    if (height_p == 0) {
        return INIT_ERR_INCORRECT_DIMENSIONS;
    }

    //get the width
    rest = strtok(NULL, "|");
    *width_p = atoi(rest);
    if (*width_p == 0) {
        return INIT_ERR_INCORRECT_DIMENSIONS;
    }

    return INIT_SUCCESS;
}

/** Takes in a string `compressed` and initializes values pointed to by
 * cells_p, width_p, and height_p accordingly. Arguments:
 *      - cells_p: a pointer to the pointer representing the cells array
 *                 that we would like to initialize.
 *      - width_p: a pointer to the width variable we'd like to initialize.
 *      - height_p: a pointer to the height variable we'd like to initialize.
 *      - snake_p: a pointer to your snake struct (not used until part 3!)
 *      - compressed: a string that contains the representation of the board.
 * Note: We assume that the string will be of the following form:
 * B24x80|E5W2E73|E5W2S1E72... To read it, we scan the string row-by-row
 * (delineated by the `|` character), and read out a letter (E, S or W) a number
 * of times dictated by the number that follows the letter.
 */
enum board_init_status decompress_board_str(int** cells_p, size_t* width_p,
                                            size_t* height_p, snake_t* snake_p,
                                            char* compressed) {
    // TODO: implement!
    if (set_dimensions(width_p, height_p, compressed) != INIT_SUCCESS) {
        return INIT_ERR_BAD_CHAR;
    }

    //mallocing cells (dereference with and height and make a board array of the size w x h)
    int* cells = malloc(*(width_p) * *(height_p) * sizeof(int));
    *cells_p = cells;

    const char* delim = "|";
    char* current_row = strtok(NULL, delim);

    int row = 0;
    int col = 0;

    while (current_row != NULL) {
        char prev_char;
        int reading_num = 0;

        int index = 0;
        //get length of current row
        int size = strlen(current_row);
        //loop through the string of that row
        for (index = 0; index <= size; index++) {
            //get character at index in the string
            char c = *(current_row + index);
            
            //if the character we are at is a letter
            if (c == 'G' || c == 'W' || c == 'E' || c == 'S' || c == '\0') {
                //fill the row if we have already seen a letter/number
                if (reading_num != 0) {
                    fill_cells(prev_char, reading_num, get_cell_pos(row, col, cells, *width_p));
                    col += reading_num;
                    //reset reading number
                    reading_num = 0;
                    if ((size_t) col >= *width_p) {
                        return INIT_ERR_INCORRECT_DIMENSIONS;
                    }
                }
                prev_char = c;
            //if it is a number then we calculate how much we need to read
            } else if ('0' <= c && c <= '9') {
                reading_num = (10 * reading_num) + ((int) (c - '0')); 
            } else {
                return INIT_ERR_BAD_CHAR;
            }

        }

        current_row = strtok(NULL, "|");
        row++;
        col = 0;
    }

    return check_board_validity(cells, *width_p, *height_p);
}

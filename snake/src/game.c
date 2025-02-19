#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

#include "linked_list.h"
#include "mbstrings.h"
#include "common.h"

/** Updates the game by a single step, and modifies the game information
 * accordingly. Arguments:
 *  - cells: a pointer to the first integer in an array of integers representing
 *    each board cell.
 *  - width: width of the board.
 *  - height: height of the board.
 *  - snake_p: pointer to your snake struct (not used until part 3!)
 *  - input: the next input.
 *  - growing: 0 if the snake does not grow on eating, 1 if it does.
 */
void update(int* cells, size_t width, size_t height, snake_t* snake_p,
            enum input_key input, int growing) {
    // `update` should update the board, your snake's data, and global
    // variables representing game information to reflect new state. If in the
    // updated position, the snake runs into a wall or itself, it will not move
    // and global variable g_game_over will be 1. Otherwise, it will be moved
    // to the new position. If the snake eats food, the game score (`g_score`)
    // increases by 1. This function assumes that the board is surrounded by
    // walls, so it does not handle the case where a snake runs off the board.

    // TODO: implement!
    if (g_game_over) {
        return;
    }

    int old_pos;
    old_pos = pos;

    if (input == INPUT_NONE) {
        input = direction;
    }

    switch (input) {
        case INPUT_DOWN:
            pos = pos + width;
            direction = INPUT_DOWN;
            break;
        case INPUT_UP:
            pos = pos - width;
            direction = INPUT_UP;
            break;
        case INPUT_LEFT:
            pos = pos - 1;
            direction = INPUT_LEFT;
            break;
        case INPUT_RIGHT:
            pos = pos + 1;
            direction = INPUT_RIGHT;
            break;
        default:
            pos = pos + 1;
            break;
    }

    //find if the new cell has a snake
    bool has_snake = cells[pos] & FLAG_SNAKE;

    //check for game over
    if (cells[pos] == FLAG_WALL || has_snake) {
        g_game_over = 1;
        return;
    } 

    //find if the new cell has food;
    bool has_food = cells[pos] & FLAG_FOOD;

    //update the place we left
    int old_cell = cells[old_pos] ^ FLAG_SNAKE;
    cells[old_pos] = old_cell;

    //update the snake with grass or not
    cells[pos] = cells[pos] | FLAG_SNAKE;

    //check for food
    if (has_food) {
        cells[pos] = cells[pos] ^ FLAG_FOOD;
        g_score = g_score + 1;
        place_food(cells, width, height);
    }
}

/** Sets a random space on the given board to food.
 * Arguments:
 *  - cells: a pointer to the first integer in an array of integers representing
 *    each board cell.
 *  - width: the width of the board
 *  - height: the height of the board
 */
void place_food(int* cells, size_t width, size_t height) {
    /* DO NOT MODIFY THIS FUNCTION */
    unsigned food_index = generate_index(width * height);
    // check that the cell is empty or only contains grass
    if ((*(cells + food_index) == PLAIN_CELL) || (*(cells + food_index) == FLAG_GRASS)) {
        *(cells + food_index) |= FLAG_FOOD;
    } else {
        place_food(cells, width, height);
    }
    /* DO NOT MODIFY THIS FUNCTION */
}

/** Prompts the user for their name and saves it in the given buffer.
 * Arguments:
 *  - `write_into`: a pointer to the buffer to be written into.
 */
void read_name(char* write_into) {
    // TODO: implement! (remove the call to strcpy once you begin your
    // implementation)
    //strcpy(write_into, "placeholder");
    printf("Name >\n");
    fflush(stdout);

    int read_count;
    while ((read_count = read(0, write_into, 1000)) != 0) {
        if (read_count < 0) {
            perror("read");
            exit(1);
        } else {
            write_into[read_count - 1] = 0;
            read_count--;
            if (read_count == 0) {
                fprintf(stderr, "Name Invalid: must be longer than 0 characters.\n");
                printf("Name >\n");
                fflush(stdout);
                continue;
            } else {
                break;
            }
        }
    }
}

/** Cleans up on game over — should free any allocated memory so that the
 * LeakSanitizer doesn't complain.
 * Arguments:
 *  - cells: a pointer to the first integer in an array of integers representing
 *    each board cell.
 *  - snake_p: a pointer to your snake struct. (not needed until part 3)
 */
void teardown(int* cells, snake_t* snake_p) {
    // TODO: implement!
}

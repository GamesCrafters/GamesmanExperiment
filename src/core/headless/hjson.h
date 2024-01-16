/**
 * @file hjson.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Json parser helper method collection for headless mode.
 * @version 1.0.0
 * @date 2024-01-15
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMESMANONE_CORE_HEADLESS_HJSON_H_
#define GAMESMANONE_CORE_HEADLESS_HJSON_H_

#include <json-c/json_object.h>  // json_object

#include "core/types/gamesman_types.h"

/**
 * @brief Adds {"position: <formal_position>"} to the DEST json object.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessJsonAddPosition(json_object *dest, ReadOnlyString formal_position);

/**
 * @brief Adds {"autoguiPosition: <autogui_position>"} to the DEST json object.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessJsonAddAutoGuiPosition(json_object *dest,
                                   ReadOnlyString autogui_position);

/**
 * @brief Adds {"move: <formal_move>"} to the DEST json object.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessJsonAddMove(json_object *dest, ReadOnlyString formal_move);

/**
 * @brief Adds {"autoguiMove: <autogui_move>"} to the DEST json object.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessJsonAddAutoGuiMove(json_object *dest, ReadOnlyString autogui_move);

/**
 * @brief Adds {"autoguiMove: <autogui_move>"} to the DEST json object.
 * @return 0 on success, non-zero error code otherwise.
 */
int HeadlessJsonAddValue(json_object *dest, Value value);
int HeadlessJsonAddRemoteness(json_object *dest, int remoteness);
int HeadlessJsonAddMovesArray(json_object *dest, json_object *moves_array_obj);

#endif  // GAMESMANONE_CORE_HEADLESS_HJSON_H_

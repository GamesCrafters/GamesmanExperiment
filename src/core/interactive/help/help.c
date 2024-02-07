#include "core/interactive/help/help.h"

#include <stdio.h>
#include <stdlib.h>  // atoi

#include "core/interactive/automenu.h"
#include "core/types/gamesman_types.h"

static ConstantReadOnlyString kHelpWhatIsGameValue =
    "A game VALUE is one of either WIN, LOSE, or TIE.  That is, if a game \n"
    "is played by two perfect opponents, the following ALWAYS happens:\n"
    "(A perfect opponent never makes a bad move, and only loses if there is\n"
    "NOTHING he/she can do about it.)\n\n"
    "WIN:\n\n"
    "The player who goes first always wins.\n\n"
    "LOSE:\n\n"
    "The player who goes first always loses.\n\n"
    "TIE:\n\n"
    "The player who goes first cannot force a win, but also cannot be\n"
    "forced into losing. The game always ends in a draw. It was a TIE\n"
    "game that caused a character from the movie 'War Games' to comment:\n\n"
    "The only way to win is not to play at all.\n\n"
    "The reason it is important to know the value of the game is that it\n"
    "determines whether going first or second is better. If the game is a\n"
    "WIN game, it is better to go first. Why? Well, theoretically, it is\n"
    "possible to win, regardless of what your opponent does. This applies\n"
    "equally to going second while playing a LOSE game. It doesn't mean that\n"
    "the person going first with a WIN game will ALWAYS win, it just says\n"
    "the potential exists. In GAMESMAN, the computer plays the part of the\n"
    "perfect opponent, but gives you, the human opponent the advantage. If\n"
    "you make a mistake, the computer pounces on you.";

static ConstantReadOnlyString kHelpWhatIsEvaluation =
    "Evaluation is the process the computer undergoes to determine the value\n"
    "of the game. (See 'What is a game VALUE?', the previous topic, for more)\n"
    "The computer searches the MOVE-TREE (the connected tree of all the moves\n"
    "that are possible from a certain position that lead to other positions)\n"
    "recursively (Depth-First Search, for the computer literate) until it \n"
    "reaches a position which it considers a PRIMITIVE win, lose or draw.\n"
    "\n"
    "A PRIMITIVE position is defined by the game itself. For example, with\n"
    "Tic-tac-toe, if the board is full, that is considered a primitive TIE.\n"
    "If a position has 3-in-a-row of X, then that position is considered a \n"
    "primitive LOSE position, because that means player O, who is staring\n"
    "blankly at the board, has just LOST. Once a primitive position has been\n"
    "reached, the algorithm backtracks in an attempt to determine the value\n"
    "of THAT position. It uses the following scheme to figure out the value\n"
    "of a non-primitive position:\n"
    "\n"
    "A non-primitive WIN position is one in which THERE EXISTS AT LEAST ONE\n"
    "move that will give my opponent a LOSE position.\n"
    "\n"
    "A non-primitive LOSE position is one in which ALL MOVES that are "
    "possible\n"
    "will give my opponent a WIN position.\n"
    "\n"
    "A non-primitive TIE position is one in which there are NO MOVES which\n"
    "will give my opponent a LOSE position, but there are SOME MOVES which\n"
    "will give my opponent another TIE position. This translates to: \n"
    "'Well, I can't win, but at least my opponent can win either'.\n"
    "\n"
    "The algorithm continues until all positions that can be reached from\n"
    "the starting position (the blank board in Tic-Tac-Toe, for instance)\n"
    "have had their value determined, and in the meantime the computer has\n"
    "figured out PERFECT paths for itself that always force you to LOSE.";

static ConstantReadOnlyString kHelpWhatArePredictions =
    "Predictions are the words in parenthesis you see when you are playing\n"
    "the game that 'predict' who will win and who will lose. Since the\n"
    "computer is a perfect opponent, if, while playing, it discovers it has\n"
    "a WIN position, there's nothing you can do, so the prediction is: "
    "'Player\n"
    "will Lose' and 'Computer will Win'. However, if you have the WIN\n"
    "position, it's always possible for you to make a really silly move and\n"
    "give the COMPUTER the WIN position, so the prediction says: 'Player\n"
    "should Win' and 'Computer should Lose'.";

static ConstantReadOnlyString kHelpWhatAreHints =
    "Hints are a list of moves that are VALUE-wise equivalent. For example,\n"
    "if you have a WIN position, then all the moves that will give your\n"
    "opponent a LOSE position are listed in the hints. If you have a LOSE\n"
    "position, ALL the possible moves are listed in the hints. The reason\n"
    "for this is explained in the help topic: 'What is EVALUATION?'. If you\n"
    "have a TIE position, the moves that give your opponent a TIE position\n"
    "are listed in the hints.";

// Constant array of read-only ConstantReadOnlyString pointers.
static const ConstantReadOnlyString *const messages[] = {
    &kHelpWhatIsGameValue,
    &kHelpWhatIsEvaluation,
    &kHelpWhatArePredictions,
    &kHelpWhatAreHints,
};

static int PrintGamesmanHelp(ReadOnlyString key) {
    int index = atoi(key);
    printf("%s\n", *(messages[index]));

    return 0;
}

int InteractiveHelp(ReadOnlyString key) {
    (void)key;  // Unused.
    static ConstantReadOnlyString kTitle = "GAMESMAN Help";
    static ConstantReadOnlyString items[] = {
        "What is a game VALUE?",
        "What is EVALUATION?",
        "What are PREDICTIONS?",
        "What are HINTS?",
    };
    static ConstantReadOnlyString keys[] = {"0", "1", "2", "3"};
    static const HookFunctionPointer hooks[] = {
        &PrintGamesmanHelp,
        &PrintGamesmanHelp,
        &PrintGamesmanHelp,
        &PrintGamesmanHelp,
    };
    int num_items = sizeof(items) / sizeof(items[0]);

    return AutoMenu(kTitle, num_items, items, keys, hooks, NULL);
}

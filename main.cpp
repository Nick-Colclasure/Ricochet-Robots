#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <algorithm>
#include <vector>

#include "board.h"
#include "boardstate.h"

// ================================================================
// ================================================================
// This function is called if there was an error with the command line arguments
bool usage(const std::string &executable_name) {
  std::cerr << "Usage: " << executable_name << " <puzzle_file>" << std::endl;
  std::cerr << "       " << executable_name << " <puzzle_file> -max_moves <#>" << std::endl;
  std::cerr << "       " << executable_name << " <puzzle_file> -all_solutions" << std::endl;
  std::cerr << "       " << executable_name << " <puzzle_file> -visualize_accessibility" << std::endl;
  std::cerr << "       " << executable_name << " <puzzle_file> -max_moves <#> -all_solutions" << std::endl;
  std::cerr << "       " << executable_name << " <puzzle_file> -max_moves <#> -visualize_accessibility" << std::endl;
  exit(0);
}


// ================================================================
// ================================================================
// load a Ricochet Robots puzzle from the input file
Board load(const std::string &executable, const std::string &filename) {

  // open the file for reading
  std::ifstream istr (filename.c_str());
  if (!istr) {
    std::cerr << "ERROR: could not open " << filename << " for reading" << std::endl;
    usage(executable);
  }

  // read in the board dimensions and create an empty board
  // (all outer edges are automatically set to be walls
  int rows,cols;
  istr >> rows >> cols;
  Board answer(rows,cols);

  // read in the other characteristics of the puzzle board
  std::string token;
  while (istr >> token) {
    if (token == "robot") {
      char a;
      int r,c;
      istr >> a >> r >> c;
      answer.placeRobot(Position(r,c),a);
    } else if (token == "vertical_wall") {
      int i;
      double j;
      istr >> i >> j;
      answer.addVerticalWall(i,j);
    } else if (token == "horizontal_wall") {
      double i;
      int j;
      istr >> i >> j;
      answer.addHorizontalWall(i,j);
    } else if (token == "goal") {
      std::string which_robot;
      int r,c;
      istr >> which_robot >> r >> c;
      answer.setGoal(which_robot,Position(r,c));
    } else {
      std::cerr << "ERROR: unknown token in the input file " << token << std::endl;
      exit(0);
    }
  }

  // return the initialized board
  return answer;
}

// ================================================================
// ================================================================

// function to calculate accessibility
std::vector<std::vector<int> > bf_accessibility(Board *board, int max_moves = -1) {
  
  std::vector<std::vector<int> > grid(board->getRows(), std::vector<int>(board->getCols(), -1));
  
  // Set initial robot positions to 0.
  for (int i = 0; i < board->numRobots(); ++i) {
    Position pos = board->getRobotPosition(i);
    grid[pos.row-1][pos.col-1] = 0;
  }

  
  BoardState initial(board);
  initial.moves.push_back(std::vector<std::pair<char, std::string> >());

  std::vector<BoardState> visited_states;
  visited_states.push_back(initial);
  
  std::queue<BoardState> queued_states;
  queued_states.push(initial);  
  // Takes a state off the queue. If it wins and we aren't looking for all
  // paths, we're done. else look at all the adjacent states and add them if we
  // thet haven't already been visited.
  while (!queued_states.empty()) {
    BoardState cur_state(queued_states.front());
    queued_states.pop();
    int move_num = cur_state.moves[0].size() + 1;
    // Stop adding states to queue after we reach max moves.
    if (move_num <= max_moves || max_moves == -1) {
      std::vector<BoardState> next_states = cur_state.get_adjacent();
      for (int i = 0; i < next_states.size(); ++i) {
        std::vector<BoardState>::iterator tmp = std::find(visited_states.begin(), visited_states.end(), next_states[i]);
        if (tmp == visited_states.end()) {
          visited_states.push_back(next_states[i]);
          queued_states.push(next_states[i]);
          for (int j = 0; j < next_states[i].bots.size(); ++j) {
            Position pos = next_states[i].bots[j];
            if (grid[pos.row-1][pos.col-1] > move_num || grid[pos.row-1][pos.col-1] == -1) {
              grid[pos.row-1][pos.col-1] = move_num;
            }
          }
        }
      // (tmp->moves[0].size() > next_states[i].moves[0].size()) should never be
      // true because the search is bredth first, meaning all moves of length
      // n should be explored at the same time, i.e. there will never be a time
      // when a state with move length n - 1 is explored after the same state
      // with move length n. Right? *TODO* Determine if this is true!
      }
    }
  }
  return grid;
}

// ================================================================
// ================================================================

// Bredth first search algorithm finding the length of one path.
std::vector<BoardState> bf_path_finder(Board *board, bool all_paths, int max_moves = -1) {
  BoardState initial(board);
  initial.moves.push_back(std::vector<std::pair<char, std::string> >());

  std::vector<BoardState> visited_states;
  visited_states.push_back(initial);
  
  std::queue<BoardState> queued_states;
  queued_states.push(initial);
  
  std::vector<BoardState> winning_states;
  
  // Takes a state off the queue. If it wins and we aren't looking for all
  // paths, we're done. else look at all the adjacent states and add them if we
  // thet haven't already been visited.
  while (!queued_states.empty()) {
    BoardState cur_state(queued_states.front());
    queued_states.pop();
    if (cur_state.wins()) {
      if (!all_paths) {
        winning_states.push_back(cur_state);
        return winning_states;
      }
      if (cur_state.moves[0].size() < max_moves || max_moves == -1)
        max_moves = cur_state.moves[0].size();
    }
    // Stop adding states to queue after we reach max moves.
    if (cur_state.moves[0].size() < max_moves) {
      std::vector<BoardState> next_states = cur_state.get_adjacent();
      for (int i = 0; i < next_states.size(); ++i) {
        std::vector<BoardState>::iterator tmp = std::find(visited_states.begin(), visited_states.end(), next_states[i]);
        if (tmp == visited_states.end()) {
          visited_states.push_back(next_states[i]);
          queued_states.push(next_states[i]);
        }
        else if (tmp->moves[0].size() == next_states[i].moves[0].size() && all_paths) {
          tmp->merge_paths(next_states[i]);
          queued_states.push(next_states[i]);
        }
      // (tmp->moves[0].size() > next_states[i].moves[0].size()) should never be
      // true because the search is bredth first, meaning all moves of length
      // n should be explored at the same time, i.e. there will never be a time
      // when a state with move length n - 1 is explored after the same state
      // with move length n. Right? *TODO* Determine if this is true!
      }
    }
  }
  std::vector<BoardState>::iterator itr = visited_states.begin();
  for (itr = visited_states.begin(); itr != visited_states.end(); ++itr) {
    if (itr->wins() && itr->moves[0].size() == max_moves) {
      winning_states.push_back(*itr);
    }
  }
  return winning_states;
}


// ================================================================
// ================================================================

int main(int argc, char* argv[]) {

  // There must be at least one command line argument, the input puzzle file
  if (argc < 2) {
    usage(argv[0]);
  }

  // By default, the maximum number of moves is unlimited
  int max_moves = -1;

  // By default, output one solution using the minimum number of moves
  bool all_solutions = false;

  // By default, do not visualize the accessibility
  bool visualize_accessibility = false;

  // Read in the other command line arguments
  for (int arg = 2; arg < argc; arg++) {
    if (argv[arg] == std::string("-all_solutions")) {
      // find all solutions to the puzzle that use the fewest number of moves
      all_solutions = true;
    } else if (argv[arg] == std::string("-max_moves")) {
      // the next command line arg is an integer, a cap on the  # of moves
      arg++;
      assert (arg < argc);
      max_moves = atoi(argv[arg]);
      assert (max_moves > 0);
    } else if (argv[arg] == std::string("-visualize_accessibility")) {
      // As a first step towards solving the whole problem, with this
      // option, let's visualize where the robots can move and how many
      // steps it takes to get there
      visualize_accessibility = true;
    } else {
      std::cout << "unknown command line argument" << argv[arg] << std::endl;
      usage(argv[0]);
    }
  }

  // Load the puzzle board from the input file
  Board board = load(argv[0],argv[1]);
  if (visualize_accessibility) {
    int rows = board.getRows();
    int cols = board.getCols();
    std::vector<std::vector<int> > access = bf_accessibility(&board, max_moves);
    std::cout << std::left;
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        if (access[i][j] != -1)
          std::cout << std::setw(3) << access[i][j];
        else
          std::cout << std::setw(3) << '.';
      }
      std::cout << std::endl;
    }
    return 0;
  }
  board.print();
  std::vector<BoardState> solutions = bf_path_finder(&board, all_solutions, max_moves);
  
  if (solutions.empty() && max_moves == -1) {
    std::cout << "no solutions" << std::endl;
    return 0;
  }
  else if (solutions.empty()) {
    std::cout << "no solutions with " << max_moves << " or fewer moves" << std::endl;
    return 0;
  }

  if (all_solutions) {
    int num_solutions = 0;
    for (int i = 0; i < solutions.size(); ++i) {
      num_solutions += solutions[i].moves.size();
    }
    std::cout << num_solutions << " different " << solutions[0].moves[0].size()
      << " move solutions" << std::endl << std::endl;
    for (int i = 0; i < solutions.size(); ++i) {
      solutions[i].print_moves();
      std::cout << std::endl;
    }
  }
  else {
    int j;
    for (j = 0; j < solutions[0].moves[0].size(); ++j) {
      std::cout << "robot " << solutions[0].moves[0][j].first << " moves " << solutions[0].moves[0][j].second << std::endl;
      board.moveRobot(board.whichRobot(solutions[0].moves[0][j].first), solutions[0].moves[0][j].second);
      board.print();
    }
    std::cout << "robot " << solutions[0].moves[0][j - 1].first << " reaches the goal after " 
    << solutions[0].moves[0].size() << " moves" << std::endl;
    std::cout << std::endl;
  }  
}

// ================================================================
// ================================================================

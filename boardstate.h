#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <utility>

#include "board.h"

#ifndef _boardstate_h_
#define _boardstate_h_

class BoardState
{
public:
  BoardState(const Board *b) : board(b), bots(b->robot_positions) {}
  // BoardState(std::vector<Position> p, const Board *b, std::vector<std::vector<std::pair<char, std::string> > > m)
  //   : bots(p), board(b), moves(m) {}
  BoardState(std::vector<Position> p, const Board *b, std::vector<std::vector<std::pair<char, std::string> > > m);
  BoardState(const BoardState &a) : board(a.board), bots(a.bots), moves(a.moves) {};
  
  BoardState & operator=(const BoardState &a);
  
  
  bool wins();
  
  bool hasRobot(Position pos) const;
  Position moveRobot(Position pos, const std::string &direction) const;
  
  // Follows edge between two nodes on the graph representing all possible states
  // on the board.
  BoardState follow_edge(int bot, std::string dir);
  std::vector<BoardState> get_adjacent();
  void print_moves();
  void merge_paths(const BoardState &b);
  
  const Board *board;
  std::vector<Position> bots;
  
  // Stores a vector of all minimum paths to this state. This is used when we
  // need to report all the minimum paths to the end.
  
  std::vector<std::vector<std::pair<char, std::string> > > moves;
    
};

bool operator==(const BoardState &a, const BoardState &b);
bool operator!=(const BoardState &a, const BoardState &b) { return !(a == b); }

// Implementation only included in file because having it in another file
// generated linker errors somehow, and I can't figure out why.

BoardState::BoardState(std::vector<Position> p, const Board *b, std::vector<std::vector<std::pair<char, std::string> > > m) {
  board = b;
  for (int i = 0; i < p.size(); ++i) {
    bots.push_back(p[i]);
  }
  for (int i = 0; i < m.size(); ++i) {
    moves.push_back(std::vector<std::pair<char, std::string> >());
    for (int j = 0; j < m[i].size(); ++j) {
      moves[i].push_back(m[i][j]);
    }
  }
}

BoardState & BoardState::operator=(const BoardState &a) {
  board = a.board;
  bots = a.bots;
  moves = a.moves;
  return *this;
}

std::ostream& operator<<(std::ostream &ostr, const BoardState &p) {
  ostr << "(";
  for (int i = 0; i < p.bots.size(); ++i) {
    ostr << " "<< p.bots[i];
  }
  ostr << " )";
  return ostr;
}


// Modified to simulate a movement of a bot at some given coordinate in
// some direction.
bool BoardState::hasRobot(Position pos) const {
  for (int i = 0; i < bots.size(); ++i) {
    if (pos == bots[i])
      return true;
  }
  return false;
}
// Gives position some robot would move to if it were to move in a given direction.
Position BoardState::moveRobot(Position pos, const std::string &direction) const {
  Position new_pos;
  if(direction == "north") {
    if (board->getHorizontalWall(pos.row - .5, pos.col) || hasRobot(Position(pos.row - 1, pos.col))) {
      return pos;
    }
    for (double i = pos.row - 1.5; i > 0; --i) {
      if (board->getHorizontalWall(i, pos.col) || hasRobot(Position(floor(i), pos.col))) {
        new_pos = Position(ceil(i), pos.col);
        break;
      }
    }
  }
  else if (direction == "east") {
    if (board->getVerticalWall(pos.row, pos.col + .5) || hasRobot(Position(pos.row, pos.col + 1))) {
      return pos;
    }
    for (double i = pos.col + 1.5; i <= board->cols + .5; ++i) {
      if (board->getVerticalWall(pos.row, i) || hasRobot(Position(pos.row, ceil(i)))) {
        new_pos = Position(pos.row, floor(i));
        break;
      }
    }
  }
  else if (direction == "south") {
    if (board->getHorizontalWall(pos.row + .5, pos.col) || hasRobot(Position(pos.row + 1, pos.col))) {
      return pos;
    }
    for (double i = pos.row + 1.5; i <= board->rows + .5; ++i) {
      if (board->getHorizontalWall(i, pos.col) || hasRobot(Position(ceil(i), pos.col))) {
        new_pos = Position(floor(i), pos.col);
        break;
      }
    }
  }
  else if (direction == "west") {
    if (board->getVerticalWall(pos.row, pos.col - .5) || hasRobot(Position(pos.row, pos.col - 1))) {
      return pos;
    }
    for (double i = pos.col - 1.5; i > 0; --i) {
      if (board->getVerticalWall(pos.row, i) || hasRobot(Position(pos.row, floor(i)))) {
        new_pos = Position(pos.row, ceil(i));
        break;
      }
    }
  }
  else {
    return pos;
  }
  return new_pos;
}

BoardState BoardState::follow_edge(int bot, std::string dir) {
  std::vector<Position> p = bots;
  p[bot] = moveRobot(bots[bot], dir);
  std::vector<std::vector<std::pair<char, std::string> > > m = moves;
  if (m.size() == 0) {
    m.push_back(std::vector<std::pair<char, std::string> >());
  }
  
  for (int i = 0; i < m.size(); ++i) {
    m[i].push_back(std::pair<char, std::string>(board->getRobot(bot), dir));
  }
    
  return BoardState(p, board, m);
}

bool BoardState::wins() {
  Position goal = board->getGoal();
  if (board->getGoalRobot() == -1) {
    for (int i = 0; i < board->numRobots(); ++i) {
      if (bots[i] == goal)
        return true;
    }
    return false;
  }
  else {
    if (bots[board->getGoalRobot()] == goal)
      return true;
    else
      return false;
  }
}

std::vector<BoardState> BoardState::get_adjacent() {
  std::vector<BoardState> res;
  for (int i = 0; i < bots.size(); ++i) {
    res.push_back(this->follow_edge(i, "north"));
    res.push_back(this->follow_edge(i, "east"));
    res.push_back(this->follow_edge(i, "south"));
    res.push_back(this->follow_edge(i, "west"));
  }
  return res;
}

void BoardState::print_moves() {
  int j;
  for (int i = 0; i < moves.size(); ++i) {
    for (j = 0; j < moves[i].size(); ++j) {
      std::cout << "robot " << moves[i][j].first << " moves " << moves[i][j].second << std::endl;
    }
    std::cout << "robot " << moves[i][j - 1].first << " reaches the goal after " 
      << moves[i].size() << " moves" << std::endl;
      std::cout << std::endl;
  }
}


void BoardState::merge_paths(const BoardState &b) {
  moves.insert(moves.end(), b.moves.begin(), b.moves.end());
}


bool operator==(const BoardState &a, const BoardState &b) {
  if (a.bots.size() != b.bots.size()) {
    return false;
  }
  for (int i = 0; i < a.bots.size(); ++i) {
    if (a.bots[i] != b.bots[i]) {
      return false;
    }
  }
  return true;
}


#endif
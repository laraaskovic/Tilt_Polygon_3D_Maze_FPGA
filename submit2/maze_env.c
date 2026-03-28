# maze_env.py
# This file defines the MAZE ENVIRONMENT used during training.
# The RL agent practices here millions of times before we put it on the FPGA.
#
# HOW RL WORKS IN ONE SENTENCE:
#   The agent tries actions, gets rewards/penalties, and slowly learns which
#   action is best in every situation, stored in a table called the Q-table.

import numpy as np
import random

# maze dimensions
COLS = 10        # tiles wide
ROWS = 10        # tiles tall
NUM_MAPS = 6         # number of different maze layouts
MAX_STEPS = 300       # max moves per episode before giving up
N_ACTIONS = 4         # 4 possible moves: up, down, left, right
                      # action 0 = up, 1 = down, 2 = left, 3 = right

# the 6 maze layouts 
# 1 = wall, 0 = open space
# These are the exact same maps in ball.c
MAPS = [
    [[1,1,1,1,1,1,1,1,1,1],
     [1,0,0,0,0,0,0,0,0,1],
     [1,0,1,1,0,0,1,1,0,1],
     [1,0,1,0,0,0,0,1,0,1],
     [1,0,0,0,1,1,0,0,0,1],
     [1,0,0,0,1,1,0,0,0,1],
     [1,0,1,0,0,0,0,1,0,1],
     [1,0,1,1,0,0,1,1,0,1],
     [1,0,0,0,0,0,0,0,0,1],
     [1,1,1,1,1,1,1,1,1,1]],

    [[1,1,1,1,1,1,1,1,1,1],
     [1,0,0,0,0,1,0,0,0,1],
     [1,0,1,1,0,1,0,1,0,1],
     [1,0,1,0,0,0,0,1,0,1],
     [1,0,1,0,1,1,1,1,0,1],
     [1,0,0,0,0,0,0,0,0,1],
     [1,1,1,1,0,1,0,1,1,1],
     [1,0,0,0,0,1,0,0,0,1],
     [1,0,1,1,1,1,0,1,0,1],
     [1,1,1,1,1,1,1,1,1,1]],

    [[1,1,1,1,1,1,1,1,1,1],
     [1,0,0,0,0,0,0,0,0,1],
     [1,0,1,1,1,1,1,1,0,1],
     [1,0,1,0,0,0,0,0,0,1],
     [1,0,1,0,1,1,0,1,0,1],
     [1,0,1,0,1,1,0,1,0,1],
     [1,0,0,0,0,0,0,1,0,1],
     [1,0,1,1,1,1,1,1,0,1],
     [1,0,0,0,0,0,0,0,0,1],
     [1,1,1,1,1,1,1,1,1,1]],

    [[1,1,1,1,1,1,1,1,1,1],
     [1,0,0,0,1,0,0,0,0,1],
     [1,0,1,0,1,0,1,1,0,1],
     [1,0,1,0,0,0,0,1,0,1],
     [1,1,1,0,1,0,0,1,0,1],
     [1,0,0,0,1,0,1,1,0,1],
     [1,0,1,1,1,0,0,0,0,1],
     [1,0,0,0,0,0,1,1,0,1],
     [1,0,1,1,0,0,0,0,0,1],
     [1,1,1,1,1,1,1,1,1,1]],

    [[1,1,1,1,1,1,1,1,1,1],
     [1,0,0,1,0,0,1,0,0,1],
     [1,0,0,0,0,0,1,0,0,1],
     [1,0,0,1,0,0,0,0,0,1],
     [1,1,0,1,1,1,1,0,1,1],
     [1,1,0,1,1,1,1,0,1,1],
     [1,0,0,0,0,0,1,0,0,1],
     [1,0,0,1,0,0,0,0,0,1],
     [1,0,0,1,0,0,1,0,0,1],
     [1,1,1,1,1,1,1,1,1,1]],

    [[1,1,1,1,1,1,1,1,1,1],
     [1,0,0,0,0,0,0,0,0,1],
     [1,0,1,0,0,1,0,0,1,1],
     [1,0,0,1,0,0,0,0,0,1],
     [1,0,0,1,0,0,0,1,0,1],
     [1,0,0,0,1,1,0,0,0,1],
     [1,0,1,0,0,0,0,0,0,1],
     [1,1,0,0,1,0,0,1,0,1],
     [1,0,0,0,1,0,0,0,0,1],
     [1,1,1,1,1,1,1,1,1,1]],
]

#  Q-table
# This is a Python dictionary that maps (state, action) -> quality score
# state = (agent_col, agent_row, target_col, target_row, map_index)
# action = 0/1/2/3
# A high Q value means "this action is good in this situation"
# A low/negative Q value means "this action is bad here"
# Starts empty, all values default to 0.0
Q = {}

def get_q(state, action):
    # look up Q value for this state+action pair
    # returns 0.0 if we have never seen this combination before
    return Q.get((state, action), 0.0)

def best_action(state):
    # look at all 4 possible actions for this state
    # return whichever has the highest Q value
    return int(np.argmax([get_q(state, a) for a in range(N_ACTIONS)]))


# MazeEnv, the virtual game the agent trains in
class MazeEnv:
    def __init__(self):
        self.reset()

    def reset(self, map_idx=None):
        # pick a random map
        self.map_idx = (map_idx % NUM_MAPS) if map_idx is not None else random.randint(0, NUM_MAPS-1)
        self.grid = MAPS[self.map_idx]

        # agent always starts at tile (col=1, row=1), top left open tile
        self.col, self.row = 1, 1

        # pick a random target tile that is:
        #   - not a wall
        #   - not the same as the start position
        while True:
            tc = random.randint(1, COLS-2)
            tr = random.randint(1, ROWS-2)
            if self.grid[tr][tc] == 0 and not (tc == 1 and tr == 1):
                break
        self.target_col, self.target_row = tc, tr

        self.steps = 0

        # visited dict tracks how many times agent has been to each tile
        # used to penalize going in circles
        self.visited = {}
        self.visited[(self.col, self.row)] = 1

        self.last_col, self.last_row = self.col, self.row
        return self._state()

    def _state(self):
        # the state is just 5 numbers the agent uses to make decisions:
        # where am I? where is the target? which map?
        return (self.col, self.row, self.target_col, self.target_row, self.map_idx)

    def step(self, action):
        # apply an action and return (new_state, reward, done)

        # calculate where we would move to
        nc, nr = self.col, self.row
        if action == 0: nr -= 1   # up = row decreases
        elif action == 1: nr += 1   # down = row increases
        elif action == 2: nc -= 1   # left = col decreases
        elif action == 3: nc += 1   # right = col increases

        self.steps += 1

        # check if the move is valid (in bounds and not a wall)
        moved = (0 <= nc < COLS and 0 <= nr < ROWS and self.grid[nr][nc] == 0)

        if moved:
            self.last_col, self.last_row = self.col, self.row
            self.col, self.row = nc, nr   # actually move

        done = False

        # reward calculation
        if not moved:
            # walked into a wall, big penalty
            reward = -1.0

        else:
            # how many times have we visited this tile before?
            visit_count = self.visited.get((self.col, self.row), 0)

            if visit_count == 0:
                # never been here, small step penalty to encourage efficiency
                reward = -0.05
            else:
                # already been here, penalty that GROWS each revisit
                # 1st revisit = -0.3, 2nd = -0.6, 3rd = -0.9 etc.
                # this is what stops the agent from oscillating back and forth which was our first iterations issue!
                reward = -0.3 * visit_count

            # record this visit
            self.visited[(self.col, self.row)] = visit_count + 1

        # check if episode is over 
        if self.col == self.target_col and self.row == self.target_row:
            # reached the target, big reward, episode ends
            reward, done = 10.0, True

        elif self.steps >= MAX_STEPS:
            # ran out of steps, penalty, episode ends
            reward, done = -2.0, True

        return self._state(), reward, done
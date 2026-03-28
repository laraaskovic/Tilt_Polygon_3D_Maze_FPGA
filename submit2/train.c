# train.py
# This file trains the RL agent using Q-learning and exports the result
# as a C header file (q_table.h) that gets compiled directly into ball.c
#
# THE BIG PICTURE:
#   1. Run thousands of episodes in the virtual maze (maze_env.py)
#   2. After each move, update the Q-table using the Bellman equation
#   3. Over time the Q-table learns the best action for every situation
#   4. Export the finished Q-table as a C array for the FPGA
#
# OUTPUT FILES:
#   q_table.h — C header included in ball.c, baked into the FPGA binary
#   q_table.json — same data in JSON, used by visualize.py to check training

import random
import numpy as np
import json
import maze_env
from maze_env import MazeEnv, N_ACTIONS, COLS, ROWS, NUM_MAPS, get_q, best_action

# training hyperparameters
EPISODES = 100000  # how many full maze runs to train on
ALPHA = 0.15 # learning rate how fast to update Q values (0=never, 1=instant)
GAMMA = 0.97  # discount factor how much to value future rewards vs immediate, 0.97 means future rewards are worth 97% of immediate ones

EPS_START = 1.0   # starting epsilon 1.0 means 100% random actions at first
EPS_END  = 0.05  # minimum epsilon always keep 5% randomness even when trained
EPS_DECAY = 0.9995  # multiply epsilon by this each episode to slowly reduce randomness

PRINT_EVERY = 500   # how often to print progress


def train():
    env = MazeEnv() # create the virtual maze
    epsilon = EPS_START  # start fully random
    rewards_log = []  # track rewards to measure progress

    for ep in range(EPISODES):
        # start a new episode
        state = env.reset()  # reset maze, get starting state
        total_reward = 0.0

        while True:
            # choose an action (epsilon-greedy strategy)
            # Early in training: mostly random (explore the maze)
            # Later in training: mostly use Q-table (exploit what we learned)
            if random.random() < epsilon:
                action = random.randint(0, N_ACTIONS-1)   # random action
            else:
                action = best_action(state)  # best known action

            # take the action, get result from environment
            next_state, reward, done = env.step(action)

            #  Q-table update (Bellman equation)
            # This is the core of Q-learning. It says:
            # "The value of this action should equal the reward we got PLUS
            #  the discounted value of the best future action from here"
            #
            # Formula: Q(s,a) = Q(s,a) + alpha * (reward + gamma * max(Q(s')) - Q(s,a))
            #
            #   old_q    = what we thought this action was worth
            #   best_nq  = the best Q value we can get from the next state
            #   target   = reward + gamma * best_nq  (what it SHOULD be worth)
            #   new_q    = old_q + alpha * (target - old_q)  (nudge toward target)

            old_q = get_q(state, action)
            best_nq = max(get_q(next_state, a) for a in range(N_ACTIONS))
            maze_env.Q[(state, action)] = old_q + ALPHA * (reward + GAMMA * best_nq - old_q)

            # move to next state
            state = next_state
            total_reward += reward

            if done:
                break   # episode over (reached target or ran out of steps)

        # decay epsilon after each episode
        # This gradually shifts from exploration to exploitation
        epsilon = max(EPS_END, epsilon * EPS_DECAY)
        rewards_log.append(total_reward)

        # print progress every 500 episodes
        if ep % PRINT_EVERY == 0:
            window = rewards_log[-100:] if len(rewards_log) >= 100 else rewards_log
            avg  = np.mean(window)

            # count episodes where reward > 5 (agent reached the target)
            wins = sum(1 for r in window if r > 5.0)
            print(f"\rEp {ep:5d}/{EPISODES} | avg: {avg:7.2f} | wins/100: {wins:3d} | eps: {epsilon:.3f}", end="", flush=True)
            print()

    print(f"\nDone! {len(maze_env.Q)} Q-table entries")

    # export to C header for FPGA
    export_c_header(maze_env.Q)

    # also save JSON so visualize.py can load it
    with open("p_table.json", "w") as f:
        json.dump({str(k): v for k, v in maze_env.Q.items()}, f)
    print("Also saved q_table.json for visualize.py")


def export_c_header(Q):
    # convert Q-table dictionary to a flat C array
    #
    # The Q-table is a Python dict: {(state, action): float}
    # C can't use Python dicts, so we convert it to a flat array of int16.
    #
    # State has 5 components: agent_col, agent_row, target_col, target_row, map
    # We pack them into a single array index like this:
    #   idx = map * (ROWS*COLS*ROWS*COLS*ACTIONS)
    #       + agent_row * (COLS*ROWS*COLS*ACTIONS)
    #       + agent_col * (ROWS*COLS*ACTIONS)
    #       + target_row * (COLS*ACTIONS)
    #       + target_col * ACTIONS
    #       + action
    #
    # Q values are floats (e.g. 7.43) but C uses int16 to save memory
    # So we multiply by 100 and round: 7.43 -> 743

    size = NUM_MAPS * ROWS * COLS * ROWS * COLS * N_ACTIONS
    table = [0] * size   # start with all zeros (unknown = neutral)

    def idx(m, ar, ac, tr, tc, a):
        # compute flat array index from 6 coordinates
        return (((m * ROWS + ar) * COLS + ac) * ROWS + tr) * COLS * N_ACTIONS + tc * N_ACTIONS + a

    # fill in the array from our trained Q-table
    for (state, action), val in Q.items():
        ac, ar, tc, tr, m = state
        i = idx(m, ar, ac, tr, tc, action)
        if 0 <= i < size:
            scaled = int(val * 100)
            scaled = max(-32767, min(32767, scaled))   # clamp to int16 range
            table[i] = scaled

    # write the C header file
    with open("p_table.h", "w") as f:
        f.write("// Auto-generated by train.py — do not edit\n")
        f.write("// Q-table: state=(agent_col, agent_row, target_col, target_row, map), action=0..3\n")
        f.write("// Values are Q-scores multiplied by 100 and stored as int16\n\n")
        f.write("#ifndef Q_TABLE_H\n#define Q_TABLE_H\n\n")

        # define constants so C code knows the dimensions
        f.write(f"#define QT_MAPS    {NUM_MAPS}\n")
        f.write(f"#define QT_ROWS    {ROWS}\n")
        f.write(f"#define QT_COLS    {COLS}\n")
        f.write(f"#define QT_ACTIONS {N_ACTIONS}\n\n")

        # the actual Q-table as a C array
        f.write(f"static const short q_table[{size}] = {{\n")
        for i in range(0, size, 16):
            chunk = table[i:i+16]
            f.write("    " + ", ".join(str(v) for v in chunk))
            if i + 16 < size:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")

        # qt_idx: converts 6 coordinates to flat array index (same formula as above)
        f.write("static inline int qt_idx(int m, int ar, int ac, int tr, int tc, int a) {\n")
        f.write("    return (((m * QT_ROWS + ar) * QT_COLS + ac) * QT_ROWS + tr)\n")
        f.write("           * QT_COLS * QT_ACTIONS + tc * QT_ACTIONS + a;\n")
        f.write("}\n\n")

        # qt_best_action: given current position and target, returns best action (0-3)
        # this is what ball.c calls every 80000 ticks to move the green agent
        f.write("static inline int qt_best_action(int ac, int ar, int tc, int tr, int m) {\n")
        f.write("    int best_a = 0;\n")
        f.write("    short best_v = q_table[qt_idx(m, ar, ac, tr, tc, 0)];\n")
        f.write("    short v;\n")
        f.write("    v = q_table[qt_idx(m, ar, ac, tr, tc, 1)]; if (v > best_v) { best_v = v; best_a = 1; }\n")
        f.write("    v = q_table[qt_idx(m, ar, ac, tr, tc, 2)]; if (v > best_v) { best_v = v; best_a = 2; }\n")
        f.write("    v = q_table[qt_idx(m, ar, ac, tr, tc, 3)]; if (v > best_v) { best_v = v; best_a = 3; }\n")
        f.write("    return best_a;\n")
        f.write("}\n\n")
        f.write("#endif // Q_TABLE_H\n")

    print(f"Exported p_table.h  ({size} entries, {size*2//1024} KB)")
    print("Copy p_table.h into your ball/ folder and recompile!")


if __name__ == "__main__":
    train()
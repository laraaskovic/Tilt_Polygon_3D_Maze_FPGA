# dfs_solver.py
# Simple DFS path generator for maze
# Returns a list of actions: 0=up,1=down,2=left,3=right

from maze_env import COLS, ROWS

# directions: up, down, left, right
DC = [0, 0, -1, 1]
DR = [-1, 1, 0, 0]

def compute_dfs_path(grid, start, target):
    visited = [[False]*COLS for _ in range(ROWS)]
    path = []

    def dfs(c, r):
        if (c, r) == target:
            return True

        visited[r][c] = True

        for a in range(4):
            nc = c + DC[a]
            nr = r + DR[a]

            if 0 <= nc < COLS and 0 <= nr < ROWS:
                if not visited[nr][nc] and grid[nr][nc] == 0:
                    path.append(a)

                    if dfs(nc, nr):
                        return True

                    path.pop()  # backtrack

        return False

    dfs(start[0], start[1])
    return path
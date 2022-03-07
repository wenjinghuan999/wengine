from ast import arg
from import_or_install import import_or_install
import math
import_or_install('cv2', 'opencv-python')
import_or_install('numpy')


def generate_chessboard(filename: str, resolution: int, n_grids: int):
    import cv2
    import numpy as np
    
    grid_res = resolution // n_grids
    if grid_res * n_grids != resolution:
        print(f'warning: there will be incomplete grid because total resolution {resolution} can not be evenly divided by grid number {n_grids}')
    B = np.zeros((grid_res, grid_res), dtype=np.float64)
    W = np.ones_like(B)
    U = np.concatenate([np.concatenate([B, W]), np.concatenate([W, B])], axis=1)
    n_units = math.ceil(resolution / grid_res / 2)
    I = np.tile(U, (n_units, n_units))
    I = I[:resolution, :resolution] * 255.0
    cv2.imwrite(filename, I)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('filename', type=str, help='output image filename')
    parser.add_argument('resolution', type=int, help='output image resolution')
    parser.add_argument('n_grids', type=int, help='number of grid in chessboard')
    args = parser.parse_args()

    # generate_chessboard('chessboard.png', 64, 4)
    generate_chessboard(args.filename, args.resolution, args.n_grids)
from import_or_install import import_or_install
import_or_install('cv2', 'opencv-python')
import_or_install('numpy')


def generate_uv_map(filename: str, resolution: int, n_grids: int):
    import cv2
    import numpy as np
    
    grid_res = resolution // n_grids
    if grid_res * n_grids != resolution:
        print(f'warning: there will be incomplete grid because total resolution {resolution} can not be evenly divided by grid number {n_grids}')
        
    def grid(u: int, v: int):
        su, sv = u >= n_grids // 2, v >= n_grids // 2
        u = u - n_grids // 2 + 1 if su else -(u - n_grids // 2)
        v = v - n_grids // 2 + 1 if sv else -(v - n_grids // 2)
        uf = u / (n_grids // 2)
        vf = v / (n_grids // 2)

        G = np.ones((grid_res, grid_res, 3), dtype=np.float64)
        G[:, :, 2] *= uf * vf if not su else 0.0
        G[:, :, 1] *= uf * vf if su and not sv else 0.0
        G[:, :, 0] *= uf * vf if sv else 0.0
        return G

    I = np.concatenate([np.concatenate([grid(u, v) for v in range(n_grids)]) for u in range(n_grids)], axis=1)
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
    generate_uv_map(args.filename, args.resolution, args.n_grids)
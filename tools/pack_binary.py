import argparse
import io


def pack_binary(in_filename: str, out_filename: str = ''):
    out = io.StringIO()
    with open(in_filename, 'rb') as f:
        content = f.read()
        for x in content:
            out.write(r'{},'.format(x))

    if out_filename:
        with open(out_filename, 'w') as f:
            f.write(out.getvalue())
    else:
        print(out.getvalue())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    parser.add_argument('--output', default='')
    args = parser.parse_args()
    pack_binary(args.input, args.output)

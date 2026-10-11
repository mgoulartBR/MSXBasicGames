"""Install the optional RAM-only MoonSound profile in an openMSX data directory."""
import argparse
from pathlib import Path
import shutil


def setup(data):
    data = Path(data)
    if not data.is_dir():
        raise ValueError('Expected existing openMSX system data directory')
    (data / 'extensions').mkdir(exist_ok=True)
    shutil.copyfile(Path(__file__).with_name('e1m1_moonsound.xml'),
                    data / 'extensions/e1m1_moonsound.xml')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data', help='existing openMSX system data directory')
    setup(parser.parse_args().data)

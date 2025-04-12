import re
import argparse
from pathlib import Path


class EpisodeExtractor:
    """used to precompile the regexps"""
    _special_patterns_str = [
        "[Ss]pecial",
        "SP[0-9]+",
        "OVA",
        "Extra",
        "Bonus"
    ]

    _episode_patterns_str = [
        "Episode[ ]*([0-9]{1,3})",         # Episode 1, Episode 12
        "Ep[ ]*([0-9]{1,3})",              # Ep 1, Ep12
        "E([0-9]{1,3})([^0-9]|$)",         # E01, E12
        "-[ ]*([0-9]{1,3})([^0-9]|$)",     # - 01, -12
        "S[0-9]+[ ]*-[ ]*([0-9]{1,3})",    # S2 - 10
        "S[0-9]+[ ]+([0-9]{1,3})",         # S2 08
        "SP[ ]*([0-9]{1,3})",              # SP01, SP 3 (for specials)
        " ([0-9]{1,2})[^0-9]"              # Fallback: isolated numbers
    ]

    special_patterns = [re.compile(p, re.IGNORECASE) for p in _special_patterns_str]
    episode_patterns = [re.compile(p, re.IGNORECASE) for p in _episode_patterns_str]

    def is_special_episode(self, filename: str) -> bool:
        for p in self.special_patterns:
            if p.search(filename):
                return True

        return False

    def extract_episode_number(self, filename: str) -> int:
        for p in self.episode_patterns:
            match = p.search(filename)
            if match:
                return int(match.group(1))

        return 0


def main(show_name: str, input_folder: Path, output_folder: Path):
    extractor = EpisodeExtractor()
    output_folder.mkdir(parents=True, exist_ok=True)

    for ext in ['mkv', 'mp4', 'avi']:
        for input_file_name in input_folder.glob(f"*.{ext}"):
            episode = extractor.extract_episode_number(str(input_file_name))
            if episode > 0:
                if extractor.is_special_episode(input_file_name):
                    (output_folder / "Specials").mkdir(parents=True, exist_ok=True)
                    output_name = output_folder / "Specials" / f"{show_name} - {episode:02} - Special.{ext}"
                else:
                    output_name = output_folder / f"{show_name} - {episode:02}.{ext}"

                print(f"{input_file_name.stem} -> {output_name}")
                input_file_name.rename(output_name)



if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Rename TV shows.',
        epilog='Example: python renamed.py -n "Showtime" -o /path/to/output'
    )

    parser.add_argument('-n', '--show-name', required=True, help='Name of the show')
    parser.add_argument('-i', '--input-folder', required=False, default=Path.cwd(), help='Input folder (default CWD)')
    parser.add_argument('-o', '--output-folder', required=True, help='Output folder')

    args = parser.parse_args()

    main(show_name=args.show_name, input_folder=Path(args.input_folder), output_folder=Path(args.output_folder))



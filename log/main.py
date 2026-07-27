import pandas as pd
from pathlib import Path

LOG_FILE = Path("./log.csv")

if __name__ == '__main__':
    if not LOG_FILE.is_file():
        print(f"[ERROR] `{LOG_FILE.as_posix()}` not found")
        exit(1)
    df = pd.read_csv(LOG_FILE)
    print(df)

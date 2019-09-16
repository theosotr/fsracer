#! /usr/bin/env python3
import argparse
import requests
from bs4 import BeautifulSoup


BASE_URL='https://www.npmjs.com/browse/depended/{!s}?offset={!s}'
H3_CLASS = 'db7ee1ac fw6 f4 black-90 dib lh-solid ma0 no-underline hover-black'


def fetch_modules(module, limit=1000, start_offset=0):
    offset = start_offset
    modules = []
    while limit > offset:
        url = BASE_URL.format(module, offset)
        response = requests.get(url)
        if response is None:
            break
        if response.status_code != 200:
            continue
        try:
            soup = BeautifulSoup(response.text, 'html.parser')
            sections = soup.findAll('section')
            modules.extend([
                sec.find('h3', {'class': H3_CLASS}).text
                for sec in sections
            ])
            offset += 36
        except:
            break
    return modules


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('module', type=str,
                        help='Module to search packages that depend-upon')
    parser.add_argument('--limit', type=int,
                        default=1000,
                        help='Maximum number of modules to retrieve')
    parser.add_argument('--start', type=int,
                        default=0,
                        help='Index to start search')
    args = parser.parse_args()
    modules = fetch_modules(args.module, limit=args.limit,
                            start_offset=args.start)
    print ('\n'.join(modules))


if __name__ == '__main__':
    main()

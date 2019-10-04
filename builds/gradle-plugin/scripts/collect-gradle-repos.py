import argparse
import requests
from bs4 import BeautifulSoup


ALL_PLUGINS_URL = 'https://plugins.gradle.org/search?page={!s}'
PLUGIN_URL = 'https://plugins.gradle.org/plugin/{!s}'
H3_CLASS = 'plugin-id'


def fetch_plugins(plugin, limit=1000, start_offset=0):
    offset = start_offset
    plugins = []
    while limit > offset:
        url = ALL_PLUGINS_URL.format(offset)
        response = requests.get(url)
        offset += 1
        if response is None:
            break
        if response.status_code != 200:
            continue
        try:
            soup = BeautifulSoup(response.text, 'html.parser')
            plugin_rows = soup.findAll('h3', {'class': H3_CLASS})
            if plugin_rows is None:
                break
            plugins.extend([
                p.find('a').text.replace('\n', '')
                for p in plugin_rows
            ])
        except:
            break
    return plugins


def fetch_plugin_repo(plugin):
    url = PLUGIN_URL.format(plugin)
    response = requests.get(url)
    if response is None:
        return None
    if response.status_code != 200:
        return None
    try:
        soup = BeautifulSoup(response.text, 'html.parser')
        plugin_website = soup.find('p', {'class': 'website'})
        return plugin_website.find('a').text
    except:
        return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('plugin', type=str,
                        help='Plugin to search')
    parser.add_argument('--limit', type=int,
                        default=1000,
                        help='Maximum number of plugins to retrieve')
    parser.add_argument('--start', type=int,
                        default=0,
                        help='Index to start search')
    args = parser.parse_args()
    if args.plugin == 'all':
        plugins = fetch_plugins(args.plugin, limit=args.limit,
                                start_offset=args.start)
        print ('\n'.join(plugins))
    else:
        repo = fetch_plugin_repo(args.plugin)
        if repo is None:
            exit(1)
        print (repo)


if __name__ == '__main__':
    main()

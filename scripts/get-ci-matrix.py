#!/usr/bin/env python3

import json
import os
import sys
import yaml

RELEASES_FILE = "RELEASES.yml"

releases = sys.argv[1:] or 'all'

print("requested releases:", releases)

releases_data = yaml.safe_load(open(RELEASES_FILE))

new_data = [r for r in releases_data['include']
        if 'all' in releases or r['release'] in releases]
matrix = {"include": new_data}

print("::set-output name=matrix::%s" % json.dumps(matrix))

#!/usr/bin/env python3

#  Copyright 2025-2026 komozoi
#  Original Creation Date: 2026-6-6
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#

import os
import sys

LICENSE_TEMPLATE = """Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License."""

EXTENSIONS = {'.cpp', '.h', '.py'}
EXCLUDE_DIRS = {'.git', 'cmake-build-debug', '_deps', 'build'}
EXCLUDE_FILES = {'picosha2.h'}

def check_file(filepath):
	if os.path.basename(filepath) in EXCLUDE_FILES:
		return True
	try:
		with open(filepath, 'r', encoding='utf-8') as f:
			content = f.read(2048) # Read the beginning of the file
			
		# Normalize template and content for checking:
		# 1. Remove comment prefixes (//, #, /*, *)
		# 2. Collapse whitespace
		
		def normalize(text):
			import re
			# Remove comment marks at the beginning of lines or after some whitespace
			text = re.sub(r'^[\s\t]*(\/\/|#|\/\*|\*|)\s*', '', text, flags=re.MULTILINE)
			# Collapse whitespace
			return " ".join(text.split())

		clean_template = normalize(LICENSE_TEMPLATE)
		clean_content = normalize(content)
		
		if clean_template in clean_content:
			return True
		return False
	except Exception as e:
		print(f"Error reading {filepath}: {e}")
		return False

def main():
	failed_files = []
	for root, dirs, files in os.walk('.'):
		# Prune excluded directories
		dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
		
		for file in files:
			if any(file.endswith(ext) for ext in EXTENSIONS):
				filepath = os.path.join(root, file)
				if not check_file(filepath):
					failed_files.append(filepath)

	if failed_files:
		print("The following files are missing the license header or have an incorrect one:")
		for f in failed_files:
			print(f"- {f}")
		sys.exit(1)
	else:
		print("All files have the correct license header.")
		sys.exit(0)

if __name__ == "__main__":
	main()

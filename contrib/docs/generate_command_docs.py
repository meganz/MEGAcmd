#!/usr/bin/env python3

import re
import subprocess
import os

MEGA_EXEC_PATH = os.environ.get('MEGA_EXEC_PATH', 'mega-exec')

def get_help_output(detail):
    det = '-ff' if detail else '-f'
    return subprocess.run([MEGA_EXEC_PATH, 'help', det, '--show-all-options'],
                          capture_output=True, text=True).stdout

def get_version():
    out = subprocess.run([MEGA_EXEC_PATH, 'version'],
                         capture_output=True, text=True).stdout
    return re.search(r'MEGAcmd version: (\d+\.\d+\.\d+)', out).group(1)

class CommandSummary:
    def __init__(self, name, args, description):
        self.name = name
        self.args = args
        self.description = description

    def get_markdown_format(self):
        args = f'`{self.args}`' if self.args else ''
        return f'* [`{self.name}`](contrib/docs/commands/{self.name}.md){args} {self.description}'

class CommandDetail:
    def __init__(self, name, summary, usage, description):
        self.name = name
        self.summary = summary
        self.usage = usage
        self.description = description

    def get_markdown_format(self):
        description = f'\n<pre>\n{self.description}\n</pre>' if self.description else ''
        return f'### {self.name}\n{self.summary}\n\nUsage: `{self.usage}`{description}'

class CommandTable:
    command_summaries = {}
    command_details = {}

    def add_command_summary(self, command_line):
        command_line = command_line.split(':', maxsplit=1)

        command_name = command_line[0]
        args = ''
        description = command_line[1]

        if ' ' in command_name:
            command_line = command_name.split(' ', maxsplit=1)
            command_name = command_line[0]
            args = command_line[1]

        command_name = command_name.strip()

        # Skip debug commands
        if command_name == 'echo':
            return

        self.command_summaries[command_name] = CommandSummary(
            name=command_name,
            args=args.strip(),
            description=description.strip()
        )

    def add_command_detail(self, command_detail):
        command_lines = command_detail.split('\n', maxsplit=3)

        # Does not start with <command_name>; not a command
        if not re.match(r'<[\w-]+>', command_lines[0]):
            return

        command_name = re.search(r'<([\w-]+)>', command_lines[0]).group(1)

        # Skip debug commands
        if command_name == 'echo':
            return

        usage = re.search(r'Usage: (.+)', command_lines[1]).group(1)
        summary = command_lines[2]
        description = command_lines[3] if len(command_lines) > 3 else ''

        command_name = command_name.strip()

        self.command_details[command_name] = CommandDetail(
            name=command_name,
            summary=summary.strip(),
            usage=usage.strip(),
            description=description.strip()
        )

    def parse_commands_brief(self):
        output = get_help_output(detail=False)

        command_section = re.split(r'Commands:\n', output, 1)[-1]
        command_lines = command_section.strip().split('\n')

        for line in command_lines:
            line = line.strip()

            # Invalid format; not a command
            if not line or not line.split(' '):
                continue

            # Doesn't start with lowercase letter; not a command
            if not re.match(r'[a-z]', line[0]):
                continue

            self.add_command_summary(line)

    def parse_commands_detail(self):
        output = get_help_output(detail=True)

        min_cols = 10
        commands = re.split(fr'-{{{min_cols},}}\n', output.strip())

        for command_detail in commands:
            command_detail = command_detail.strip()

            # Invalid format; not a command
            if not command_detail:
                continue

            self.add_command_detail(command_detail)

def get_commands_by_category(ct):
    categories = {
        'Account / Contacts': ['signup', 'confirm', 'invite', 'showpcr', 'ipc', 'users', 'userattr', 'passwd', 'masterkey'],
        'Login / Logout': ['login', 'logout', 'whoami', 'session', 'killsession'],
        'Browse': ['cd', 'lcd', 'ls', 'pwd', 'lpwd', 'attr', 'du', 'find', 'mount'],
        'Moving / Copying files': ['mkdir', 'cp', 'put', 'get', 'preview', 'thumbnail', 'mv', 'rm', 'transfers', 'speedlimit', 'sync', 'sync-issues', 'sync-ignore', 'sync-config', 'exclude', 'backup'],
        'Sharing (your own files, of course, without infringing any copyright)': ['export', 'import', 'share', 'webdav'],
        'FUSE (mount your cloud folder to the local system)': ['fuse-add', 'fuse-remove', 'fuse-enable', 'fuse-disable', 'fuse-show', 'fuse-config'],
    }

    # Commands that don't have a explicit group should go into Misc.
    commands =  set([c for l in categories.values() for c in l])
    misc_commands = set(ct.command_summaries.keys()) - commands
    categories['Misc.'] = sorted(misc_commands)

    return categories

def generate_user_guide_summary(ct):
    categories = get_commands_by_category(ct)

    summary = []
    for category, commands in categories.items():
        summary.append('### ' + category)
        for command in commands:
            cs = ct.command_summaries[command]
            summary.append(cs.get_markdown_format())
        summary.append('')

    return '\n'.join(summary) + '\n'

def write_to_user_guide(summary):
    with open('UserGuide.md', 'r') as f:
        contents = f.read()

    # Write MEGAcmd version
    version_sentence = 'This document relates to MEGAcmd version '
    contents = re.sub(version_sentence + r'\d+\.\d+\.\d+', version_sentence + get_version(), contents)

    # Write summary of all commands
    anchor = '<<<<<<<<<<<<<<<<ANCHOR>>>>>>>>>>>>>>>>'
    contents = contents.replace('### Account / Contacts\n', anchor)
    contents = contents.replace('## Examples', anchor + '## Examples')

    contents = contents.split(anchor)
    contents = contents[0] + summary + contents[2]

    with open('UserGuide.md', 'w') as f:
        f.write(contents)

def write_command_files(command_details):
    for cd in command_details:
        with open(f'contrib/docs/commands/{cd.name}.md', 'w') as f:
            f.write(cd.get_markdown_format() + '\n')

if __name__=='__main__':
    ct = CommandTable()
    ct.parse_commands_brief()
    ct.parse_commands_detail()

    user_guide_summary = generate_user_guide_summary(ct)
    write_to_user_guide(user_guide_summary)

    write_command_files(ct.command_details.values())

# hif - How I Feel

An emotional command line interface.

## What?

![shrug](/media/shrug.png?raw=true "shrug")

I wanted a quick and easy command line interface to track my thoughts and
feelings throughout the day in a way that felt natural to me. I spend a large
portion of my day on the CLI, so I created `hif` to help me document my
feelings over time.

As `hif` grew, I also added an easy way to create short journal entries (memos)
to the `hif` database.

## Requirements

1. sqlite3 libraries

## Building

```bash
$ ./autogen.sh
$ ./configure
$ make
```

## How?
usage: `hif [+emotion | command (args)*]`

### Emotion Commands
	add {emotion}        - Journal a new {emotion} feel.
	                       alias +, i.e. $ hif +sad
	delete-feel {id}     - Delete a feel by id.

### Journaling Commands
	memo {memo}          - Add a memo.
	delete-memo {memo-id}- Delete a memo by id.

### Metadata Commands
	describe-feel {feel} - Describe a feel.
	create-emotion       - Create a new emotion.
	count-feels          - Return a count of feels.
	create-context       - Create a new feels context database.

### Import/Export Commands
	export-json          - Dump feels in json format.

	help                 - Print this message.
	version              - Print hif version information.

## Examples

Creating a new emotion:

```bash
$ hif create-emotion "love" "I love you"
```

Journaling a happy feel:
```bash
$ hif +love
```

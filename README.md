---
title: "hif - How I Feel"
---

# An Emotional Command Line Interface.

Hi, my name is Jason and I made a small command line interface to track
my day-to-day feelings.

I wanted something that would allow me to throw a feeling into a database with
very little effort. Here's what I came up with:

```bash
$ hif +happy
```

It's important that I track my feelings over time to help me cope with a mental
health issue. Some people keep journals, use web sites or apps, etc, which is
pretty cool and useful! For me, however, I spend so much time on the command
line that it's often too much of a context switch to open a book or reference a
phone.

Right now `hif` is very primative... but I often add features that are useful
to me.

Maybe they'll be useful to you, too.

## Where?

![shrug](media/shrug.png?raw=true "shrug")

`hif` source code is available on github, here: [https://github.com/jgshort/hif](https://github.com/jgshort/hif).

Conveniently, I also track issues and features on github.

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
	delete-memo {id}     - Delete a memo by id.

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

### License

<a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-sa/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/">Creative Commons Attribution-ShareAlike 4.0 International License</a>.


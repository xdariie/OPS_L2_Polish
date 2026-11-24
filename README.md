# L2: Rat Mail

## Task Description

An undead merchant is enjoying an influx of customers into her small shop. Each of these unfortunate people sincerely, but vainly, believes that they will manage to lift the curse. Since the surrounding toxic swamp lies on the way to this noble goal, everyone stocks up on blooming moss. Business is going so well that the undead merchant can no longer collect this moss herself and must use the services of other suppliers. The rats will handle the logistics, but they do not have a convenient user interface for displaying shipments and messages.

According to the Rat King's instructions, rats store a collection of data about letters and shipments in `.smbox` files. Each such file is a sequence of variable-sized blocks with the following structure:

- 1 byte: total size of the header in bytes (including the terminating `'\0'`), stored in binary format, within the range [2, 255].
- 1 byte: total size of the message body in bytes (including the terminating `'\0'`), stored in binary format, within the range [2, 255].
- A character string terminated with `'\0'`: the message header.
- A character string terminated with `'\0'`: the message body.

The user interface for displaying the contents of the file must support the following commands:

- `'w'`: select the previous message (if the selected message is not the first one), execute `'h'`.
- `'s'`: select the next message (if the selected message is not the last one), execute `'h'`.
- `'m'`: print the header and body of the currently selected message on two separate lines.
- `'h'`: print the headers of all messages in the file, additionally displaying `'-> '` before the header of the selected message.
- `'q'`: release all resources and close the application.
- `'d'`: print the entire contents of the file as text. This command will be removed in the release version.

The program is executed as follows:

```bash
./sop-mbox <smbox_file> [fifo...]
```

## Stages

### 1. [6 points]

Read commands from standard input in a non-blocking manner. In a loop, attempt to read at most `BATCH_BYTES` bytes. If any data is available, process every character read; otherwise, wait 100 ms before the next attempt. If the user enters an unsupported character (except for the newline character `'\n'`), print `"Invalid input"`.

At this stage, `'w'` and `'s'` print the number of the currently selected message (even if it remains unchanged), assuming a range from 0 to infinity, e.g., `"Selected message: 0"`.

`'d'`, `'h'`, and `'m'` print, respectively, `"Dumping the file..."`, `"Printing headers..."`, and `"Printing message <selected message number>"`.

The `'q'` command is fully functional.

### 2. [5 points]

Implement the `'d'` command by loading the entire file using `mmap()` and printing it using `write()`. Ensure that the program cannot modify the file.

**Hint:** You can use the `MAP_PRIVATE` flag.

The printed text will not look correct; do not worry about it. However, it is worth remembering the necessity of closing the file and using `munmap()`, as well as considering what may result from the absence of `fflush()` between `printf()` and `write()`.

*Stages 3 and 4 are on the next page...*

### 3. [8 points]

Complete the remaining commands. This time, it will be necessary to parse the file at the beginning of the program. Terminate the program if parsing fails.

Use `writev()` to print headers and messages.

### 4. [5 points]

The optional program arguments are paths to individual FIFO files representing additional input devices. Simultaneous input of commands from standard input and a maximum of `MAX_FIFOS` FIFO files must be supported, while minimizing CPU usage.

Take into account that a process writing commands to a FIFO may disconnect and reconnect through the same file.

**Hint:** Continuous reporting of the `POLLHUP` event can be eliminated by replacing the file descriptor.

The `mkfifos.sh` script creates `MAX_FIFOS` FIFO files. `write_to_fifos.sh` writes commands to these files one after another.

This stage can be tested as follows:

```bash
(sleep 1 && ./scripts/write_to_fifos.sh) & ./sop-mbox <file> [fifo1] [fifo2] ...
```
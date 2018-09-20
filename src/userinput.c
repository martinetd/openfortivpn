/*
 *  Copyright (C) 2015 Davíð Steinn Geirsson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "userinput.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int read_password(const char *prompt, char **pass)
{
	int masked = 0;
	struct termios oldt, newt;
	char *line = NULL;
	size_t linesz = 0;
	int rc;

	printf("%s", prompt);
	fflush(stdout);

	// Try to hide user input
	if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
		newt = oldt;
		newt.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		masked = 1;
	}

	rc = getline(&line, &linesz, stdin);

	if (masked)
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	printf("\n");

	if (rc < 0) {
		rc = errno;
		free(line);
		return rc;
	}
	line[rc ? rc - 1 : rc] = '\0';
	free(*pass);
	*pass = line;

	return 0;
}

/*
 *  Copyright (C) 2015 Adrien Vergé
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "log.h"

/*
 * Adds a sha256 digest to the list of trusted certificates.
 */
int add_trusted_cert(struct vpn_config *cfg, const char *digest)
{
	struct x509_digest *last, *new;

	new = malloc(sizeof(struct x509_digest));
	if (new == NULL)
		return 1;

	new->next = NULL;
	strncpy(new->data, digest, SHA256STRLEN - 1);

	if (cfg->cert_whitelist == NULL) {
		cfg->cert_whitelist = new;
	} else {
		for (last = cfg->cert_whitelist; last->next != NULL;
		     last = last->next) ;
		last->next = new;
	}

	return 0;
}

int load_config(struct vpn_config *cfg, const char *filename)
{
	int ret = -1;
	FILE *file;
	struct stat stat;
	char *buffer, *line;

	file = fopen(filename, "r");
	if (file == NULL) {
		log_error("fopen: %s\n", strerror(errno));
		return 1;
	}

	if (fstat(fileno(file), &stat) == -1) {
		log_error("fstat: %s\n", strerror(errno));
		goto err_close;
	}
	if (stat.st_size == 0)
		goto err_close;

	buffer = malloc(stat.st_size);
	if (buffer == NULL) {
		log_error("malloc failed.\n");
		goto err_close;
	}

	// Copy all file contents at once
	if (fread(buffer, stat.st_size, 1, file) != 1) {
		log_error("fread failed\n");
		goto err_free;
	}

	// Read line by line
	for (line = strtok(buffer, "\n"); line != NULL;
	     line = strtok(NULL, "\n")) {
		char *key, *equals, *val;
		int i;

		if (line[0] == '#')
			continue;

		// Expect something like: "key = value"
		equals = strchr(line, '=');
		if (equals == NULL) {
			log_warn("Bad line in config file.\n");
			goto err_free;
		}
		equals[0] = '\0';
		key = line;
		val = equals + 1;

		// Remove heading spaces
		while (key[0] != '\0' && (key[0] == ' ' || key[0] == '\t'))
			key++;
		while (val[0] != '\0' && (val[0] == ' ' || val[0] == '\t'))
			val++;
		// Remove trailing spaces
		for (i = strlen(key) - 1; i > 0; i--) {
			if (key[i] == ' ' || key[i] == '\t')
				key[i] = '\0';
			else
				break;
		}
		for (i = strlen(val) - 1; i > 0; i--) {
			if (val[i] == ' ' || val[i] == '\t')
				val[i] = '\0';
			else
				break;
		}

		if (strcmp(key, "host") == 0) {
			strncpy(cfg->gateway_host, val, FIELD_SIZE - 1);
		} else if (strcmp(key, "port") == 0) {
			long int port = strtol(val, NULL, 0);
			if (port <= 0 || port > 65535) {
				log_warn("Bad port in config file: \"%d\".\n",
					 port);
				continue;
			}
			cfg->gateway_port = port;
		} else if (strcmp(key, "username") == 0) {
			strncpy(cfg->username, val, FIELD_SIZE - 1);
		} else if (strcmp(key, "password") == 0) {
			strncpy(cfg->password, val, FIELD_SIZE - 1);
		} else if (strcmp(key, "trusted-cert") == 0) {
			if (strlen(val) != SHA256STRLEN)
				log_warn("Bad certificate sha256 digest in "
					 "config file: \"%s\".\n", val);
				continue;
			}
			if (add_trusted_cert(cfg, val)) {
				log_warn("Could not add certificate digest to "
					 "whitelist.\n");
		} else {
			log_warn("Bad key in config file: \"%s\".\n", key);
			goto err_free;
		}
	}

	ret = 0;

err_free:
	free(buffer);
err_close:
	fclose(file);

	return ret;
}
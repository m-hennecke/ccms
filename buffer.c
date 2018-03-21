
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"

struct buffer *
buffer_new(char *_data)
{
	size_t data_len = strlen(_data);
	struct buffer *buf = malloc(sizeof(struct buffer) + data_len);
	buf->size = data_len;
	strlcpy(buf->data, _data, data_len + 1);
	return buf;
}


struct buffer *
buffer_bin_new(void *_data, size_t _size)
{
	struct buffer *buf = malloc(sizeof(struct buffer) + _size);
	buf->size = _size;
	memcpy(buf->data, _data, _size);
	return buf;
}


struct buffer_list *
buffer_list_new(void)
{
	struct buffer_list *bl = malloc(sizeof(struct buffer_list));
	if (bl == NULL)
		err(1, NULL);
	TAILQ_INIT(&bl->buffers);
	bl->size = 0;
	return bl;
}


void
buffer_list_free(struct buffer_list *_bl)
{
	if (_bl) {
		struct buffer *b;
		while ((b = buffer_list_rem_head(_bl)))
			free(b);
	}
}


ssize_t
buffer_write(struct buffer *_buf, int _fd)
{
	size_t off;
	ssize_t nw;

	for (off = 0; off < _buf->size; off += nw) {
		if ((nw = write(_fd, _buf->data + off, _buf->size - off)) == 0
				|| nw == -1) {
			warn("write");
			return -1;
		}
	}
	return _buf->size;
}


char *
buffer_list_concat_string(struct buffer_list *_bl)
{
	struct buffer *buf;

	char *out_string = malloc(_bl->size + 1);
	out_string[0] = '\0';
	TAILQ_FOREACH(buf, &_bl->buffers, entries) {
		strlcat(out_string, buf->data, _bl->size + 1);
	}

	return out_string;
}


void
buffer_list_add_string(struct buffer_list *_bl, char *_s)
{
	struct buffer *buf = buffer_new(_s);
	TAILQ_INSERT_TAIL(&_bl->buffers, buf, entries);
	_bl->size += buf->size;
}


void
buffer_list_add(struct buffer_list *_bl, void *_data, size_t _size)
{
	struct buffer *buf = buffer_bin_new(_data, _size);
	TAILQ_INSERT_TAIL(&_bl->buffers, buf, entries);
	_bl->size += buf->size;
}


struct buffer *
buffer_list_rem_head(struct buffer_list *_bl)
{
	if (! TAILQ_EMPTY(&_bl->buffers)) {
		struct buffer *b = TAILQ_FIRST(&_bl->buffers);
		_bl->size -= b->size;
		TAILQ_REMOVE(&_bl->buffers, b, entries);
		return b;
	}
	return NULL;
}


struct buffer *
buffer_list_rem_tail(struct buffer_list *_bl)
{
	if (! TAILQ_EMPTY(&_bl->buffers)) {
		struct buffer *b = TAILQ_LAST(&_bl->buffers, buffer_list_head);
		_bl->size -= b->size;
		TAILQ_REMOVE(&_bl->buffers, b, entries);
		return b;
	}
	return NULL;
}

struct buffer *
buffer_list_rem(struct buffer_list *_bl, struct buffer *_buf)
{
	struct buffer *b;
	TAILQ_FOREACH(b, &_bl->buffers, entries) {
		if (b == _buf) {
			_bl->size -= _buf->size;
			TAILQ_REMOVE(&_bl->buffers, b, entries);
			return b;
		}
	}
	return _buf;
}

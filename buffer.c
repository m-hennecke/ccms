
#include <endian.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "buffer.h"


#define DEFLATE_BUF_SIZE	0x4000
#define GZIP_ENCODING		16
#define OS_CODE			0x03
#define ORIG_NAME		0x08

struct buffer *
buffer_new(const char *_data)
{
	size_t data_len = strlen(_data);
	struct buffer *buf = malloc(sizeof(struct buffer) + data_len);
	if (buf == NULL)
		err(1, NULL);
	buf->size = data_len;
	strlcpy(buf->data, _data, data_len + 1);
	return buf;
}


struct buffer *
buffer_bin_new(const void *_data, size_t _size)
{
	struct buffer *buf = malloc(sizeof(struct buffer) + _size);
	if (buf == NULL)
		err(1, NULL);
	buf->size = _size;
	memcpy(buf->data, _data, _size);
	return buf;
}


struct buffer *
buffer_empty_new(size_t _size)
{
	struct buffer *buf = malloc(sizeof(struct buffer) + _size);
	if (buf == NULL)
		err(1, NULL);
	buf->size = _size;
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
	if (out_string == NULL)
		err(1, NULL);
	out_string[0] = '\0';
	TAILQ_FOREACH(buf, &_bl->buffers, entries) {
		strlcat(out_string, buf->data, _bl->size + 1);
	}

	return out_string;
}


char *
buffer_list_concat(struct buffer_list *_bl)
{
	struct buffer *buf;

	char *out = malloc(_bl->size);
	if (out == NULL)
		err(1, NULL);
	char *next_out = out;
	TAILQ_FOREACH(buf, &_bl->buffers, entries) {
		memcpy(next_out, buf->data, _bl->size);
		next_out += buf->size;
	}

	return out;
}


void
buffer_list_add_string(struct buffer_list *_bl, const char *_s)
{
	struct buffer *buf = buffer_new(_s);
	TAILQ_INSERT_TAIL(&_bl->buffers, buf, entries);
	_bl->size += buf->size;
}


void
buffer_list_add(struct buffer_list *_bl, const void *_data, size_t _size)
{
	struct buffer *buf = buffer_bin_new(_data, _size);
	TAILQ_INSERT_TAIL(&_bl->buffers, buf, entries);
	_bl->size += buf->size;
}


void
buffer_list_add_buffer(struct buffer_list *_bl, struct buffer *_buf)
{
	TAILQ_INSERT_TAIL(&_bl->buffers, _buf, entries);
	_bl->size += _buf->size;
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


struct buffer_list *
buffer_list_gzip(struct buffer_list *_in, const char *_name, uint32_t _mtime)
{
	z_stream strm = { 0 };
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	u_int32_t crc = crc32(0L, Z_NULL, 0);

	int rc;
	if ((rc = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED,
				-MAX_WBITS, 9,
				Z_DEFAULT_STRATEGY)) != Z_OK)
		errx(1, "deflateInit2: %s", zError(rc));

	struct buffer *input, *output;
	struct buffer_list *out = buffer_list_new();

	unsigned char gzip_header[10];
	gzip_header[0] = 0x1f;
	gzip_header[1] = 0x8b;
	gzip_header[2] = Z_DEFLATED;
	gzip_header[3] = (_name) ? ORIG_NAME : 0;
	gzip_header[4] = _mtime & 0xff;
	gzip_header[5] = _mtime >> 8;
	gzip_header[6] = _mtime >> 16;
	gzip_header[7] = _mtime >> 24;
	gzip_header[8] = 2;
	gzip_header[9] = OS_CODE;
	buffer_list_add(out, gzip_header, sizeof(gzip_header));

	if (_name)
		buffer_list_add(out, _name, strlen(_name) + 1);

	output = buffer_empty_new(DEFLATE_BUF_SIZE);
	strm.total_in = _in->size;
	strm.next_out = output->data;
	strm.avail_out = DEFLATE_BUF_SIZE;

	TAILQ_FOREACH(input, &_in->buffers, entries) {
		strm.avail_in = input->size;
		strm.next_in = input->data;

		while (strm.avail_in) {
			if ((rc = deflate(&strm, Z_NO_FLUSH)) != Z_OK)
				errx(1, "deflate: %s", zError(rc));
			if (strm.avail_out == 0) {
				buffer_list_add_buffer(out, output);
				output = buffer_empty_new(DEFLATE_BUF_SIZE);
				strm.next_out = output->data;
				strm.avail_out = DEFLATE_BUF_SIZE;
			}
		}
		crc = crc32(crc, input->data, input->size);
	}
	if ((rc = deflate(&strm, Z_FINISH)) != Z_STREAM_END)
		errx(1, "deflate: %s", zError(rc));

	if ((rc = deflateEnd(&strm)) != Z_OK)
		errx(1, "deflateEnd: %s", zError(rc));

	size_t last_buf_size = DEFLATE_BUF_SIZE - strm.avail_out;
	if (last_buf_size != 0) {
		buffer_list_add(out, output->data, last_buf_size);
		u_int32_t y = htole32(crc);
		buffer_list_add(out, &y, sizeof(u_int32_t));
		u_int32_t s = htole32(_in->size & 0xffffffff);
		buffer_list_add(out, &s, sizeof(u_int32_t));
	}

	free(output);

	return out;
}

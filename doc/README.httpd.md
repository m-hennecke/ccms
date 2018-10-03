# OpenBSD httpd configuration notes

## Example configuration for httpd

Include the directories /var/www/htdocs/{images,css} to be served by httpd,
setup location /cgi-bin/ to pass requests to slowcgi and request a rewrite
for `/*.html` style URLs: 


```
server "example.com" {
	listen on * port 80
	root "/htdocs"
	location "/css/*" {
		pass
	}
	location "/downloads/*" {
		pass
	}
	location "/images/*" {
		pass
	}
	location "/sitemap.xml.gz" {
		request rewrite "/cgi-bin/sitemap/sitemap.xml.gz"
	}
	location "/sitemap.xml" {
		request rewrite "/cgi-bin/sitemap/sitemap.xml"
	}
	location "/cgi-bin/*" {
		fastcgi
		root "/"
	}
	location "/*.html" {
		request rewrite "/cgi-bin/cms$REQUEST_URI"
	}
	location "/" {
		request rewrite "/cgi-bin/cms"
	}
}


```

## chroot for httpd

Remember to set the `CHROOT` variable to /var/www if using httpd(8) and
slowcgi(8).


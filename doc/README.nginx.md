# nginx configuration notes

## Example configuration for nginx

Include the directories /var/www/htdocs/{images,css} to be served by nginx,
setup location `/.*\.html` to pass requests to slowcgi:


```
       location /sitemap.xml.gz {
            fastcgi_pass unix:/run/slowcgi.sock;
            fastcgi_param SCRIPT_FILENAME /cgi-bin/sitemap;
            fastcgi_param PATH_INFO /sitemap.xml.gz;
            include fastcgi_params;
            allow all;
       }
       location ~ /(.*\.html)$ {
            fastcgi_pass unix:/run/slowcgi.sock;
            fastcgi_param SCRIPT_FILENAME /cgi-bin/cms;
            fastcgi_param PATH_INFO $1;
            include fastcgi_params;
            allow all;
        }

```

In case of `CMS_ROOT_URL` not being the default `/` the location block
above have to be adjusted accordingly.

Add location settings for `/css/`, `/downloads/` and `/images/`.

## chroot for slowcgi

Remember to set the `CHROOT` variable to /var/www  using nginx(8) and
slowcgi(8).


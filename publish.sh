#!/bin/sh

source ./.env

rm -rf ./public
mkdir ./public ./public/media

cp pandoc.css public/

pandoc -s --toc -c pandoc.css -o public/index.html README.md
cp media/* public/media/

aws s3 sync --acl "public-read" --sse "AES256" public/ ${destination} --profile=${profile}
aws cloudfront create-invalidation --distribution-id ${distribution} --paths /index.html / /media/* --profile=${profile}


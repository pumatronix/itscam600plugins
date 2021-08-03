FROM python:3.9-alpine

WORKDIR /usr/src/app

COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY server.py .

# -u option will make stdout/stderr readily available
# in 'docker logs <CONTAINER_ID> --tail 10 -f'
CMD [ "python", "-u", "server.py" ]

EXPOSE 6000/tcp

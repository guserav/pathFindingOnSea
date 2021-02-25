FROM python:3.9-buster

RUN apt-get update && apt-get install -y \
    python3-flask \
    pkg-config \
    libosmium2-dev

COPY . /fapraOSM
WORKDIR fapraOSM
ENV FLASK_APP=app.py
ENV FLASK_ENV=development
RUN make
EXPOSE 5000

CMD flask run --host=0.0.0.0

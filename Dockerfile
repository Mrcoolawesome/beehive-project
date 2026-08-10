FROM python:3.12-slim

ARG UID=1000
ARG GID=1000

RUN groupadd -g "${GID}" beehive \
    && useradd -m -u "${UID}" -g "${GID}" beehive

WORKDIR /app

# Only the fprime tooling requirements are needed to run the GDS - the
# top-level requirements.txt also pulls in hardware-specific deps (evdev)
# that are only needed on the Raspberry Pi running the flight binary.
COPY lib/fprime/requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir -r /tmp/requirements.txt

USER beehive

# 50000: comm link the flight binary (running on the Raspberry Pi) connects into
# 5000: GDS web GUI
EXPOSE 50000 5000

ENTRYPOINT ["fprime-gds"]
CMD ["--no-app", \
     "-d", "/app/build-artifacts/arm-hf-linux/BeeDeployment", \
     "--gui-addr", "0.0.0.0", \
     "--ip-address", "0.0.0.0", \
     "--file-storage-directory", "/data", \
     "-l", "/app/logs"]

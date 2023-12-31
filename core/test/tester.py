import string
import random

import matplotlib.pyplot as plt
import os

import subprocess

FILENAME = "C:/Users/vyach/CLionProjects/llp-lab1/test.bin"
INSERT_TEST = "cmake-build-windows/test/test_insert.exe"
SELECT_TEST = "cmake-build-windows/test/test_select.exe"
SELECT_WHERE_TEST = "cmake-build-windows/test/test_where.exe"
DELETE_TEST = "cmake-build-windows/test/test_delete.exe"
UPDATE_TEST = "cmake-build-windows/test/test_update.exe"
READ_TEST = "cmake-build-windows/test/test_read.exe"
JOIN_TEST = "cmake-build-windows/test/test_join.exe"
CREATE_TABLE_TEST = "cmake-build-windows/test/test_createtable.exe"


def file_size():
    return os.path.getsize("C:/Users/vyach/CLionProjects/llp-lab1/test.bin")


def random_string(n: int) -> str:
    return ''.join(random.choice(string.digits + string.ascii_letters) for _ in range(n))


def random_int32() -> int:
    return random.randint(-2147483647, 2147483646)


def random_bool() -> int:
    return random.choice([0, 1])


def random_float() -> float:
    return round(random.uniform(-1, 1), 7)


def random_input() -> str:
    return f"{random_int32()} {random_string(32)} {random_bool()} {random_float()} "


def call_insert(n: int, mode: int, records: list[str], table_name: str) -> float:
    p = subprocess.Popen([INSERT_TEST, str(n), str(mode), table_name],
                         stdout=subprocess.PIPE,
                         stdin=subprocess.PIPE)
    written = []
    for i in range(n):
        written.append(records[i])
        p.stdin.write(bytes(records[i], 'ascii'))
    p.stdin.flush()
    took = float(p.stdout.read())
    ret = p.wait(None)
    assert ret == 0
    return took


def call_select() -> float:
    p = subprocess.Popen([SELECT_WHERE_TEST], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    took = float(p.stdout.read())
    ret = p.wait(None)
    assert ret == 0
    return took


def call_join() -> float:
    p = subprocess.Popen([JOIN_TEST], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    took = float(p.stdout.read())
    ret = p.wait(None)
    assert ret == 0
    return took


def call_delete() -> float:
    p = subprocess.Popen([DELETE_TEST], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    took = float(p.stdout.read())
    ret = p.wait(None)
    assert ret == 0
    return took


def call_update() -> float:
    p = subprocess.Popen([UPDATE_TEST], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    took = float(p.stdout.read())
    ret = p.wait(None)
    assert ret == 0
    return took


def call_read() -> list[str]:
    p = subprocess.Popen([READ_TEST], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    lines = [bytes.decode(i, encoding="ascii") for i in p.stdout.readlines()]
    ret = p.wait(None)
    assert ret == 0
    return lines


def call_create_table(mode: int, table_name: str):
    p = subprocess.Popen([CREATE_TABLE_TEST, str(mode), table_name], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    ret = p.wait(None)
    assert ret == 0


def test_insert(table_name: str):
    call_insert(0, 2, [], table_name)
    y = []
    x = []
    z = []
    batch_size = 1000
    for i in range(1000):
        timediff = call_insert(batch_size, 1, [random_input() for _ in range(batch_size)], table_name)
        size = file_size()
        # print(f"n: {i}, inserting: {batch_size}, took: {timediff}s, filesize: {size}")
        y.append(timediff)
        x.append(i)
        z.append(size)
    fig, (time_axes, size_axes) = plt.subplots(2, sharex=True)

    time_axes.set_title(f"Insert Test (batch size: {batch_size} records)")
    time_axes.plot(x, y)
    time_axes.set_ylabel("time (seconds)")

    size_axes.plot(x, z)
    size_axes.set_ylabel("file size (bytes)")
    size_axes.set_xlabel("batch number")

    plt.show()


def test_select():
    batch_size = 1000
    y = []
    x = []
    call_insert(0, 2, [], "test")
    for i in range(100):
        print(i)
        call_insert(batch_size, 1, [random_input() for _ in range(batch_size)], "test")
        timediff = call_select()
        y.append(timediff)
        x.append(i * batch_size)

    fig, time_axes = plt.subplots()

    time_axes.set_title(f"Select Test")
    time_axes.plot(x, y)
    time_axes.set_ylabel("time (seconds)")
    time_axes.set_xlabel("records number")

    plt.show()


def test_delete_time():
    batch_size = 1000
    y = []
    x = []
    for i in range(1, 101):
        print(i)
        call_insert(i * batch_size, 2, [random_input() for _ in range(batch_size * i)], "test")
        timediff = call_delete()
        y.append(timediff)
        x.append(i * batch_size)

    fig, time_axes = plt.subplots()

    time_axes.set_title(f"Delete Test")
    time_axes.plot(x, y)
    time_axes.set_ylabel("time (seconds)")
    time_axes.set_xlabel("records number")

    plt.show()


def test_update_time():
    batch_size = 1000
    y = []
    x = []
    call_insert(0, 2, [], "test")
    for i in range(0, 100):
        print(i)
        call_insert(batch_size, 1, [random_input() for _ in range(batch_size)], "test")
        timediff = call_update()
        y.append(timediff)
        x.append(i * batch_size)

    fig, time_axes = plt.subplots()

    time_axes.set_title(f"Update Test")
    time_axes.plot(x, y)
    time_axes.set_ylabel("time (seconds)")
    time_axes.set_xlabel("records number")

    plt.show()


def test_delete_size():
    batch_size = 1000
    y = []
    x = []
    call_insert(0, 2, [], "test")
    for i in range(100):
        print(i)
        call_insert(
            batch_size,
            1,
            [f"{random_int32()} {random_string(32)} {1} {random_float()} " for _ in range(batch_size)],
            "test"
        )
        call_delete()
        y.append(file_size())
        x.append(i)

    fig, time_axes = plt.subplots()

    time_axes.set_title(f"Delete-Insert Test")
    time_axes.plot(x, y)
    time_axes.set_ylabel("file size (bytes)")
    time_axes.set_xlabel("records number")

    plt.show()


def test_update_size():
    batch_size = 1000
    y = []
    x = []
    call_insert(
        batch_size,
        2,
        [f"{random_int32()} {random_string(32)} {1} {random_float()} " for _ in range(batch_size)],
        "test"
    )
    for i in range(100):
        print(i)
        call_update()
        y.append(file_size())
        x.append(i)

    fig, time_axes = plt.subplots()

    time_axes.set_title(f"Insert-Update Test")
    time_axes.plot(x, y)
    time_axes.set_ylabel("file size (bytes)")
    time_axes.set_xlabel("records number")

    plt.show()


def test_join():
    batch_size = 10
    y = []
    x = []
    call_create_table(2, "test")
    call_create_table(1, "test2")
    for i in range(50):
        print(i)
        call_insert(batch_size, 1, [random_input() for _ in range(batch_size)], "test")
        call_insert(batch_size, 1, [random_input() for _ in range(batch_size)], "test2")
        timediff = call_join()
        y.append(timediff)
        x.append(i * batch_size)

    fig, time_axes = plt.subplots()

    time_axes.set_title(f"Select-Join Test")
    time_axes.plot(x, y)
    time_axes.set_ylabel("time (seconds)")
    time_axes.set_xlabel("records number")

    plt.show()


def correctness(n: int):
    data = {i: (i, random_string(32), random_bool(), random_float()) for i in range(n)}
    call_insert(n, 2, [f"{i[0]} {i[1]} {i[2]} {i[3]} " for i in data.values()], "test")

    res = call_read()
    read_data = []
    for i in res:
        row_str = i.strip().split()
        row = (int(row_str[0]), row_str[1], int(row_str[2]), float(row_str[3]))
        read_data.append(row)

    # check correctness
    assert(len(read_data) == len(data))
    for read, real in zip(read_data, data.values()):
        assert read[0] == real[0]
        assert read[1] == real[1]
        assert read[2] == real[2]
        assert abs(read[3] - real[3]) < 0.001


if __name__ == '__main__':
    test_join()

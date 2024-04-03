import socket
import struct
import math
import pygame


class MyCell:
    def __init__(self, ids, xs, ys, mass_t):
        self.id = ids
        self.x = xs
        self.y = ys
        self.mass = mass_t


# Create a socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Define the port
port = 5000

# connect to the server
s.connect(('localhost', port))

# receive hello from the server
print(s.recv(1024).decode())

# receive starting cell
buffer = s.recv(28)
myID, x, y, mass = struct.unpack('i d d i', buffer)
myCell = MyCell(myID, x, y, mass)

pygame.init()

WIDTH = 1200
HEIGHT = 800
FPS = 60
SPEED = 5

screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Agar.io")
clock = pygame.time.Clock()
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    mx, my = pygame.mouse.get_pos()

    dx = mx - myCell.x
    dy = my - myCell.y
    angle = math.atan2(dy, dx)

    myCell.x += int(math.cos(angle) * SPEED)
    myCell.y += int(math.sin(angle) * SPEED)

    # send position change to the server
    data = struct.pack('i d d i', myCell.id, myCell.x, myCell.y, myCell.mass)
    s.sendall(data)

    # Draw everything
    screen.fill((0, 0, 0))
    pygame.draw.circle(screen, (255, 255, 255), (int(myCell.x), int(myCell.y)), 25)

    # Flip the display
    pygame.display.flip()

    # Cap the frame rate
    clock.tick(FPS)

# close the connection
s.close()
import socket
import struct
import math
import pygame


class MyCell:
    def __init__(self, id, x, y, mass_t, color_t):
        self.id = id
        self.x = x
        self.y = y
        self.mass = mass_t
        self.color = color_t


class Food:
    def __init__(self, x, y):
        self.x = x
        self.y = y


class GUI:
    def __init__(self):
        self.myCell = None
        self.enemies = []
        self.foods = []
        self.screen = None
        self.clock = None
        self.running = None
        self.speed = 5
        self.fps = 60

        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        port = 5000
        self.s.connect(('localhost', port))

    def receive_cell(self):
        buffer = self.s.recv(40)
        myID, x, y, mass, r, g, b = struct.unpack('i d d i i i i', buffer)
        self.myCell = MyCell(myID, x, y, mass, (r, g, b))

    def init_pygame(self):
        pygame.init()
        width = 1200
        height = 800
        self.screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("Agar.io")
        self.clock = pygame.time.Clock()
        self.running = True

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False

    def update_cell_position(self):
        mx, my = pygame.mouse.get_pos()
        dx = mx - self.myCell.x
        dy = my - self.myCell.y
        distance = math.sqrt(dx ** 2 + dy ** 2)
        self.radius = math.sqrt(self.myCell.mass / math.pi)
        if distance > self.radius:
            angle = math.atan2(dy, dx)
            self.myCell.x += int(math.cos(angle) * self.speed)
            self.myCell.y += int(math.sin(angle) * self.speed)

    def send_position_change(self):
        data = struct.pack('i d d i i i i', self.myCell.id, self.myCell.x, self.myCell.y,
                           self.myCell.mass, self.myCell.color[0], self.myCell.color[1], self.myCell.color[2])
        self.s.sendall(data)

    def get_enemies_position(self):
        self.enemies = []
        buffer = self.s.recv(4)
        (enemies_number,) = struct.unpack("i", buffer)
        for i in range(enemies_number):
            buffer = self.s.recv(40)
            myID, x, y, mass, r, g, b, enemies_number = struct.unpack('i d d i i i i', buffer)
            self.enemies.append(MyCell(myID, x, y, mass, (r, g, b)))

    def draw_everything(self):
        self.screen.fill((255, 255, 255))
        # pygame.draw.circle(self.screen, self.myCell.color, (int(self.myCell.x), int(self.myCell.y)), self.radius) i tak narysuje gracza
        for cell in self.enemies:
            pygame.draw.circle(self.screen, cell.color, (int(cell.x), int(cell.y)), math.sqrt(cell.mass / math.pi))
        for food in self.foods:
            pygame.draw.circle(self.screen, (255,0,0), (int(food.x), int(food.y)), math.sqrt(200 / math.pi))
        pygame.display.flip()
        self.clock.tick(self.fps)

    def recieve_int(self):
        buffer = self.s.recv(4)
        number = struct.unpack("i", buffer)[0]
        return number

    def upload_map(self):

        # clear buffers
        self.enemies = []
        self.foods = []

        # get enemies
        enemies_n = self.recieve_int()
        for i in range(enemies_n):
            buffer = self.s.recv(40)
            myID, x, y, mass, r, g, b = struct.unpack('i d d i i i i', buffer)
            self.enemies.append(MyCell(myID, x, y, mass, (r, g, b)))

        # get foods
        for i in range(10):
            buffer = self.s.recv(16)
            x, y = struct.unpack('d d', buffer)
            self.foods.append(Food(x, y))

    def run(self):
        self.receive_cell()
        self.init_pygame()
        while self.running:
            self.handle_events()
            self.update_cell_position()
            self.send_position_change()
            self.upload_map()
            self.receive_cell()
            self.draw_everything()
        # end connection
        self.myCell.id = -1
        self.send_position_change()
        self.s.close()


def main():
    gui = GUI()
    gui.run()
    return


if __name__ == "__main__":
    main()

import tkinter as tk
from tkinter import filedialog
import sys
import ast
import logging, os, sys

# Get the folder where your EXE (or script) lives
exe_dir = os.path.dirname(sys.executable if getattr(sys, 'frozen', False) else __file__)
log_path = os.path.join(exe_dir, 'app.log')

logging.basicConfig(
    filename=log_path,
    level=logging.DEBUG,
    format='%(asctime)s %(levelname)s: %(message)s'
)

# --- File‐chooser at startup --------------------------------------
root = tk.Tk()
root.withdraw()  # hide main window while file dialog is open

file_path = filedialog.askopenfilename(
    title="Select game data file",
    filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
)
if not file_path:
    logging.debug("No file selected. Exiting.")
    root.destroy()
    sys.exit()

root.deiconify()  # re-show main window
# -----------------------------------------------------------------

# holds the list of actions for the current hand
current_actions = []
# index of the next action to fire
action_index    = 0

# --- Load all lines once for initial config parse ----------------
try:
    with open(file_path) as f:
        all_lines = f.readlines()
except Exception as e:
    logging.debug(f"Error loading file '{file_path}':", e)
    all_lines = []
# -----------------------------------------------------------------

# --- Functions ----------------------------------------------------
def show_input():
    hand_number = entry.get()
    label.config(text=f"Showing hand {hand_number}")
    process_hand_with_delay(int(hand_number))

def process_hand_with_delay(hand_number):
    global current_actions, action_index, community_index, pot

    reset_display()
    community_index = 0
    pot             = 0

    # use the chosen file_path here
    try:
        with open(file_path) as f:
            lines = f.readlines()
    except FileNotFoundError:
        label.config(text=f"File not found:\n{file_path}")
        return

    # Find start of hand
    start_index = None
    for i, line in enumerate(lines):
        if line.strip() == f"hand:{hand_number}":
            start_index = i
            break

    if start_index is None:
        label.config(text=f"Hand {hand_number} not found.")
        return

    # Parse stacks line for this hand
    for i in range(start_index + 1, len(lines)):
        line = lines[i].strip()
        if line.startswith("Stacks:"):
            try:
                stack_list = ast.literal_eval(line.replace("Stacks:", "").strip())
                for j, amount in enumerate(stack_list):
                    player_pots[j] = amount
                    color = "gray" if amount == 0 else "green"
                    canvas.itemconfig(player_rectangles[j], fill=color)
                    canvas.itemconfig(player_action_texts[j], text="" if amount == 0 else "XX-X : XX-X")
                    canvas.itemconfig(player_pot_texts[j], text=f"${amount}" if amount else "")
                    canvas.itemconfig(player_blind_texts[j], text="")
                break
            except Exception as e:
                logging.debug("Error parsing stacks:", e)
                break

    # collect actions into our global list
    current_actions = []
    for i in range(start_index + 1, len(lines)):
        line = lines[i].strip()
        if line.startswith("hand:"):
            break
        current_actions.append(line)

    # reset the pointer
    action_index = 0

def next_action():
    global action_index
    if action_index < len(current_actions):
        perform_action(current_actions[action_index])
        action_index += 1
    else:
        label.config(text="▶ No more actions")

def reset_display():
    for i in range(numPlayers):
        canvas.itemconfig(player_rectangles[i], fill="green", width=1)
        canvas.itemconfig(player_action_texts[i], text="XX-X : XX-X")
        canvas.itemconfig(player_blind_texts[i], text="")
        canvas.itemconfig(player_pot_texts[i], text=f"${player_pots[i]}")
    for i in range(5):
        canvas.itemconfig(community_card_texts[i], text="XX-X")
    canvas.itemconfig(action_display, text="")
    canvas.itemconfig(pot_display, text="POT: $0")
    for pid in player_hands:
        player_hands[pid] = "XX-X : XX-X"

def perform_action(action):
    global pot, community_index

    if action.startswith("dealer:"):
        idx = int(action.replace("dealer:", "").strip())
        canvas.itemconfig(player_blind_texts[idx], text="D")

    elif action.startswith("com:"):
        cards = action.replace("com:", "").split(",")
        for card in cards:
            if community_index < 5:
                canvas.itemconfig(community_card_texts[community_index], text=card)
                community_index += 1

    elif action.startswith("p") and ":" in action:
        pid, move = action.split(":")
        if pid in player_ids:
            idx = player_ids.index(pid)

            if move.startswith("c-") or move.startswith("r-"):
                parts = move.split("-")
                if len(parts) == 2 and parts[1].isdigit():
                    amount = int(parts[1])
                    player_pots[idx] -= amount
                    pot += amount
                    canvas.itemconfig(player_pot_texts[idx], text=f"${player_pots[idx]}")
                    label = "CALLS" if parts[0] == "c" else "RAISES"
                    canvas.itemconfig(action_display, text=f"{pid.upper()} {label} ${amount}")
                    canvas.itemconfig(pot_display, text=f"POT: ${pot}")
                else:
                    canvas.itemconfig(action_display, text=f"{pid.upper()} {move.upper()}")

            elif "-" in move and move.count("-") == 1 and len(move) <= 4:
                if player_hands[pid] == "XX-X : XX-X":
                    player_hands[pid] = f"{move} : XX-X"
                else:
                    first = player_hands[pid].split(" : ")[0]
                    player_hands[pid] = f"{first} : {move}"
                canvas.itemconfig(player_action_texts[idx], text=player_hands[pid])

            elif move == "F":
                canvas.itemconfig(player_rectangles[idx], fill="red")
                canvas.itemconfig(action_display, text=f"{pid.upper()} folds")

            elif move.startswith("a-"):
                pot += int(move[2::])
                player_pots[idx] = 0
                canvas.itemconfig(player_rectangles[idx], fill="purple")
                canvas.itemconfig(action_display, text=f"{pid.upper()} all in")
                canvas.itemconfig(player_pot_texts[idx], text=f"${player_pots[idx]}")
                canvas.itemconfig(pot_display, text=f"POT: ${pot}")

            elif move in ["sb", "bb"]:
                blind_value = sb if move == "sb" else bb
                player_pots[idx] -= blind_value
                pot += blind_value
                canvas.itemconfig(player_pot_texts[idx], text=f"${player_pots[idx]}")
                canvas.itemconfig(player_blind_texts[idx], text=move.upper())
                canvas.itemconfig(action_display, text=f"{pid.upper()} posts {move.upper()}")
                canvas.itemconfig(pot_display, text=f"POT: ${pot}")

    elif action.startswith("W-p"):
        idx = int(action[3]) - 1
        parts = action.split(":")
        player_pots[idx] += int(parts[1])
        pot -= int(parts[1])
        canvas.itemconfig(player_rectangles[idx], fill="gold")
        canvas.itemconfig(action_display, text=f"P{idx + 1} wins!")
        canvas.itemconfig(player_pot_texts[idx], text=f"${player_pots[idx]}")
        canvas.itemconfig(pot_display, text=f"POT: {pot}")
    elif action.startswith("Winner"):
        idx = int(action[8]) - 1
        winnings = int(action[10::])
        
        for x in range (numPlayers):
            if x == idx:
                canvas.itemconfig(player_rectangles[x], fill="gold")
            else:
                canvas.itemconfig(player_rectangles[x], fill = "red")
        canvas.itemconfig(action_display, text=f"P{idx + 1} WINNER : GAME OVER")
    elif action.startswith("Stacks") == False:
        logging.debug("Unkown Action: " + action)
        

# -----------------------------------------------------------------

# --- GUI SETUP ---------------------------------------------------
root.title("NFC POKER GAME")
root.geometry("600x600")

label = tk.Label(root, text="Which Hand would you like to view:")
label.pack(pady=10)

entry = tk.Entry(root, width=30)
entry.pack(pady=10)

button = tk.Button(root, text="Submit", command=show_input)
button.pack(pady=10)

next_btn = tk.Button(root, text="Next Action", command=next_action)
next_btn.pack(pady=5)

game_info_label = tk.Label(root)
game_info_label.pack(pady=5)

canvas = tk.Canvas(root, width=400, height=400, bg="white")
canvas.pack()

# --- Parse initial config from all_lines (instead of GameId.txt) ---
numPlayers, pot, sb, bb = 6, 1000, 10, 20
for line in all_lines:
    line = line.strip()
    if line == "Game Start":
        break
    if line.startswith("players:"):
        numPlayers = int(line.split(":")[1])
    elif line.startswith("pot:"):
        pot = int(line.split(":")[1])
    elif line.startswith("sb:"):
        sb = int(line.split(":")[1])
    elif line.startswith("bb:"):
        bb = int(line.split(":")[1])

game_info_label.config(
    text=f"Players: {numPlayers}   Starting Pot: ${pot}   SB: ${sb}   BB: ${bb}"
)

player_ids = [f"p{i+1}" for i in range(numPlayers)]
player_rectangles = []
player_action_texts = []
player_pot_texts = []
player_blind_texts = []
player_pots = [pot] * numPlayers
player_hands = {pid: "XX-X : XX-X" for pid in player_ids}

positions = [
    (0, 0, 75, 75, 37, 15, 30),
    (163, 0, 237, 75, 200, 15, 30),
    (325, 0, 400, 75, 363, 15, 30),
    (0, 325, 75, 400, 37, 385, 370),
    (163, 325, 237, 400, 200, 385, 370),
    (325, 325, 400, 400, 363, 385, 370)
][:numPlayers]

for i, pos in enumerate(positions):
    rect = canvas.create_rectangle(*pos[:4], fill="green")
    player_rectangles.append(rect)
    canvas.create_text(pos[4], pos[5], text=f"P{i+1}", font=("Arial", 16), fill="blue")
    pot_text = canvas.create_text(pos[4], pos[6], text=f"${pot}", font=("Arial", 12), fill="blue")
    player_pot_texts.append(pot_text)
    action_text = canvas.create_text(
        pos[4], pos[6] + 15 if i < 3 else pos[6] - 15,
        text="XX-X : XX-X", font=("Arial", 10), fill="blue"
    )
    player_action_texts.append(action_text)
    blind_text = canvas.create_text(
        pos[4], pos[6] + 30 if i < 3 else pos[6] - 30,
        text="", font=("Arial", 10), fill="purple"
    )
    player_blind_texts.append(blind_text)

community_card_texts = []
for x in [50, 125, 200, 275, 350]:
    community_card_texts.append(
        canvas.create_text(x, 200, text="XX-X", font=("Arial", 20), fill="blue")
    )

community_index = 0
action_display = canvas.create_text(200, 250, text="", font=("Arial", 15), fill="blue")
pot_display = canvas.create_text(200, 150, text=f"POT: ${pot}", font=("Arial", 15), fill="blue")

root.mainloop()

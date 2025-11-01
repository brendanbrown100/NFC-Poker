from django.shortcuts import render, redirect, get_object_or_404
from django.contrib.auth.decorators import login_required
from django.contrib import messages
from .models import Game
from .forms import GameForm


@login_required
def index(request):
    """Display all games for the logged-in user"""
    games = Game.objects.filter(user=request.user)
    
    # Calculate total profit
    total_profit = sum(game.profit for game in games)
    
    template_data = {
        'title': 'My Poker Games',
        'games': games,
        'total_profit': total_profit,
        'game_count': games.count()
    }
    return render(request, 'games/index.html', {'template_data': template_data})


@login_required
def add_game(request):
    """Add a new game"""
    if request.method == 'POST':
        form = GameForm(request.POST, request.FILES)
        if form.is_valid():
            game = form.save(commit=False)
            game.user = request.user
            
            # Parse game data to calculate profit
            if game.game_data:
                try:
                    game_data_content = game.game_data.read().decode('utf-8')
                    game.game_data.seek(0)  # Reset file pointer
                    game_details = parse_game_data(game_data_content)
                    
                    # Calculate profit based on player number
                    game.profit = calculate_player_profit(game_details, game.player_number)
                    
                    # Auto-generate name if not provided
                    if not game.name:
                        game.name = f"Game {game.date.strftime('%m/%d/%Y')}"
                        
                except Exception as e:
                    messages.error(request, f'Error parsing game data: {str(e)}')
                    return render(request, 'games/add_game.html', {'template_data': {'title': 'Add New Game', 'form': form}})
            
            game.save()
            messages.success(request, f'Game "{game.name}" added successfully! Your profit/loss: ${game.profit}')
            return redirect('games.index')
    else:
        form = GameForm()
    
    template_data = {
        'title': 'Add New Game',
        'form': form
    }
    return render(request, 'games/add_game.html', {'template_data': template_data})

@login_required
def view_game(request, game_id):
    """View details of a specific game"""
    game = get_object_or_404(Game, id=game_id, user=request.user)
    
    # Parse game data if file exists
    game_details = None
    if game.game_data:
        try:
            game_data_content = game.game_data.read().decode('utf-8')
            game_details = parse_game_data(game_data_content)
        except Exception as e:
            messages.warning(request, f'Could not parse game data: {str(e)}')
    
    template_data = {
        'title': f'Game #{game.id}',
        'game': game,
        'game_details': game_details
    }
    return render(request, 'games/view_game.html', {'template_data': template_data})


@login_required
def delete_game(request, game_id):
    """Delete a specific game"""
    game = get_object_or_404(Game, id=game_id, user=request.user)
    
    if request.method == 'POST':
        game_name = game.name
        
        # Delete file if it exists
        if game.game_data:
            try:
                game.game_data.delete(save=False)
            except Exception:
                pass  # Ignore file deletion errors
        
        game.delete()
        messages.success(request, f'Game "{game_name}" deleted successfully!')
        return redirect('games.index')
    
    template_data = {
        'title': 'Delete Game',
        'game': game
    }
    return render(request, 'games/delete_game.html', {'template_data': template_data})


def parse_game_data(content):
    """Parse the game data from the Arduino-generated text file"""
    lines = content.strip().split('\n')
    data = {
        'players': 0,
        'starting_pot': 0,
        'small_blind': 0,
        'big_blind': 0,
        'hands': [],
        'winner': None,
        'final_stacks': {}
    }
    
    current_hand = None
    
    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        if line.startswith('players:'):
            data['players'] = int(line.split(':')[1])
        elif line.startswith('pot:'):
            data['starting_pot'] = int(line.split(':')[1])
        elif line.startswith('sb:'):
            data['small_blind'] = int(line.split(':')[1])
        elif line.startswith('bb:'):
            data['big_blind'] = int(line.split(':')[1])
        elif line.startswith('hand:'):
            if current_hand:
                data['hands'].append(current_hand)
            current_hand = {
                'hand_number': int(line.split(':')[1]),
                'dealer': None,
                'stacks': [],
                'actions': [],
                'winners': [],
                'bets': {}  # Track total bets per player {player_num: total_bet}
            }
        elif line.startswith('dealer:') and current_hand:
            current_hand['dealer'] = int(line.split(':')[1])
        elif line.startswith('Stacks:') and current_hand:
            # Parse stacks like: Stacks:[1000,1000,1000]
            stacks_str = line.split('[')[1].split(']')[0]
            current_hand['stacks'] = [int(x) for x in stacks_str.split(',')]
        elif line.startswith('W-p') and current_hand:
            # Parse winner like: W-p3:450
            parts = line.split(':')
            player = int(parts[0].replace('W-p', ''))
            amount = int(parts[1])
            current_hand['winners'].append({'player': player, 'amount': amount})
        elif line.startswith('Winner:p'):
            # Final winner - parse like "Winner:p1-2500"
            parts = line.split(':')[1].split('-')
            player_num = int(parts[0].replace('p', ''))
            final_chips = int(parts[1])
            data['winner'] = {
                'player': player_num,
                'final_chips': final_chips
            }
            data['final_stacks'][player_num] = final_chips
        elif current_hand and ':' in line:
            # Parse betting actions like: p3:sb, p3:bb, p3:c-100, p3:r-50, p3:F
            parts = line.split(':')
            if len(parts) == 2 and parts[0].startswith('p'):
                try:
                    player_num = int(parts[0].replace('p', ''))
                    action = parts[1]
                    
                    # Initialize bet tracking for this player
                    if player_num not in current_hand['bets']:
                        current_hand['bets'][player_num] = 0
                    
                    # Parse bet amounts from actions
                    if action == 'sb':
                        current_hand['bets'][player_num] += data['small_blind']
                    elif action == 'bb':
                        current_hand['bets'][player_num] += data['big_blind']
                    elif action.startswith('c-'):
                        # Call: c-50 means call 50 more
                        bet_amount = int(action.split('-')[1])
                        current_hand['bets'][player_num] += bet_amount
                    elif action.startswith('r-'):
                        # Raise: r-100 means raise by 100 (additional bet)
                        bet_amount = int(action.split('-')[1])
                        current_hand['bets'][player_num] += bet_amount
                    # f-X, b-X could be fold/bet variants - ignore for now
                    
                    current_hand['actions'].append(line)
                except (ValueError, IndexError):
                    current_hand['actions'].append(line)
            else:
                current_hand['actions'].append(line)
        elif current_hand:
            current_hand['actions'].append(line)
    
    if current_hand:
        data['hands'].append(current_hand)
    
    return data


def calculate_player_profit(game_details, player_number):
    """Calculate profit/loss for a specific player"""
    starting_pot = game_details.get('starting_pot', 0)
    
    # Method 1: If we have final winner data with exact chips
    if game_details.get('final_stacks') and player_number in game_details['final_stacks']:
        final_chips = game_details['final_stacks'][player_number]
        profit = final_chips - starting_pot
        return profit
    
    # Method 2: Calculate using hand-by-hand stacks and the next hand's starting stack
    # The "Stacks" line in each hand shows chips AFTER the previous hand completed
    hands = game_details.get('hands', [])
    
    if not hands:
        return 0
    
    # If there are multiple hands, we can use the next hand's stack to see the result
    final_chips = starting_pot  # Start with initial amount
    
    for i, hand in enumerate(hands):
        # Check if there's a next hand
        if i + 1 < len(hands):
            # The next hand's Stacks line shows the result after this hand
            next_hand = hands[i + 1]
            if next_hand.get('stacks') and len(next_hand['stacks']) >= player_number:
                final_chips = next_hand['stacks'][player_number - 1]
        else:
            # This is the last hand - we need to calculate
            # Start with this hand's beginning stack
            if hand.get('stacks') and len(hand['stacks']) >= player_number:
                starting_stack_this_hand = hand['stacks'][player_number - 1]
                
                # Find winnings in this hand
                winnings = 0
                for winner_info in hand.get('winners', []):
                    if winner_info['player'] == player_number:
                        winnings += winner_info['amount']
                
                # Find total bets in this hand
                bets = hand.get('bets', {}).get(player_number, 0)
                
                # Final chips = starting stack of this hand - bets + winnings
                final_chips = starting_stack_this_hand - bets + winnings
    
    profit = final_chips - starting_pot
    return profit
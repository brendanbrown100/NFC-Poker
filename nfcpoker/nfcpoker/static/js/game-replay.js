// === NFC Poker Multi-Hand Replay (Fixed Version) ===
// Each hand has its own replay window with canvas + action log below.

class PokerMultiReplay {
  constructor(gameData, containerId) {
    this.gameData = gameData;
    this.container = document.getElementById(containerId);
    if (!this.container) {
      console.error(`❌ PokerMultiReplay: container "${containerId}" not found.`);
      return;
    }
    
    // Extract which hand this is from the data
    this.handIndex = 0;
    if (this.gameData.hands && this.gameData.hands.length > 0) {
      this.handIndex = this.gameData.hands[0].hand_number - 1;
    }
    
    this.renderHand(); 
  }
  
  renderHand() {
    const hand = this.gameData.hands[0]; // We only have one hand in this instance
    const idx = this.handIndex;
    
    this.container.innerHTML = `
      <div class="card shadow-sm mb-3">
        <div class="card-header bg-dark text-white py-2">
          <strong>Hand ${hand.hand_number}</strong>
          ${hand.dealer !== null && hand.dealer !== undefined ? 
            `<span class="text-light ms-2">(Dealer: P${hand.dealer + 1})</span>` : ''}
        </div>
        <div class="card-body p-3" style="background:#f8f9fa;">
          <canvas id="canvas-${idx}" width="550" height="400" class="border mb-2"></canvas>

          <div id="action-log-${idx}" class="alert alert-secondary small py-2 mb-2">
            <strong>Action:</strong> <span id="current-action-${idx}">Ready to replay...</span>
          </div>

          <div class="btn-group btn-group-sm mb-3">
            <button class="btn btn-success" id="next-${idx}">
              <i class="fas fa-step-forward"></i> Next
            </button>
            <button class="btn btn-info" id="auto-${idx}">
              <i class="fas fa-play"></i> Auto
            </button>
            <button class="btn btn-warning" id="reset-${idx}">
              <i class="fas fa-redo"></i> Reset
            </button>
          </div>

          <div id="summary-${idx}" class="small border-top pt-2" style="max-height:120px; overflow-y:auto;">
            <em>Actions will appear here...</em>
          </div>
        </div>
      </div>
    `;

    // Initialize replay controller for this specific hand
    const replay = new PokerHandReplay(this.gameData, hand, idx);
    document.getElementById(`next-${idx}`).onclick = () => replay.nextAction();
    document.getElementById(`auto-${idx}`).onclick = () => replay.autoPlay();
    document.getElementById(`reset-${idx}`).onclick = () => replay.reset();
  }
}

// === Individual Hand Replay Logic ===
class PokerHandReplay {
  constructor(gameData, hand, index) {
    this.gameData = gameData;
    this.hand = hand;
    this.index = index;
    this.canvas = document.getElementById(`canvas-${index}`);
    
    if (!this.canvas) {
      console.error(`❌ Canvas not found for index ${index}`);
      return;
    }
    
    this.ctx = this.canvas.getContext('2d');

    this.actions = [];
    this.currentActionIndex = 0;
    this.communityIndex = 0;
    this.communityCards = ['XX-X', 'XX-X', 'XX-X', 'XX-X', 'XX-X'];
    this.playerHands = {};
    
    // Initialize player pots from hand stacks
    this.playerPots = hand.stacks ? [...hand.stacks] : 
                      Array(gameData.players).fill(gameData.starting_pot);
    
    this.playerStates = Array(gameData.players).fill('active');
    this.pot = 0;

    // Initialize player hands
    for (let i = 0; i < gameData.players; i++) {
      this.playerHands[`p${i + 1}`] = 'XX-X : XX-X';
    }

    this.setupActions();
    this.setPositions();
    this.drawTable();
  }

  setupActions() {
    // Add dealer action
    if (this.hand.dealer !== null && this.hand.dealer !== undefined) {
      this.actions.push({ type: 'dealer', value: this.hand.dealer });
    }
    
    // Add all game actions
    if (this.hand.actions && Array.isArray(this.hand.actions)) {
      this.hand.actions.forEach(a => this.actions.push({ type: 'raw', value: a }));
    }
    
    // Add winner actions
    if (this.hand.winners && Array.isArray(this.hand.winners)) {
      this.hand.winners.forEach(w => {
        this.actions.push({ 
          type: 'winner', 
          player: w.player, 
          amount: w.amount 
        });
      });
    }
  }

  setPositions() {
    // Player positions on canvas
    this.playerPositions = [
      { x: 80, y: 60 },
      { x: 250, y: 40 },
      { x: 420, y: 60 },
      { x: 80, y: 340 },
      { x: 250, y: 360 },
      { x: 420, y: 340 }
    ].slice(0, this.gameData.players);

    // Community card positions
    this.communityPositions = [
      { x: 130, y: 200 },
      { x: 210, y: 200 },
      { x: 290, y: 200 },
      { x: 370, y: 200 },
      { x: 450, y: 200 }
    ];
  }

  reset() {
    this.currentActionIndex = 0;
    this.communityIndex = 0;
    this.communityCards = ['XX-X', 'XX-X', 'XX-X', 'XX-X', 'XX-X'];
    this.playerPots = this.hand.stacks ? [...this.hand.stacks] : 
                      Array(this.gameData.players).fill(this.gameData.starting_pot);
    this.playerStates.fill('active');
    this.pot = 0;
    
    // Reset player hands
    for (let i = 0; i < this.gameData.players; i++) {
      this.playerHands[`p${i + 1}`] = 'XX-X : XX-X';
    }
    
    document.getElementById(`summary-${this.index}`).innerHTML = '<em>Reset. Click Next to begin.</em>';
    document.getElementById(`current-action-${this.index}`).textContent = 'Ready to replay...';
    this.drawTable();
  }

  nextAction() {
    if (this.currentActionIndex >= this.actions.length) {
      document.getElementById(`current-action-${this.index}`).innerHTML = 
        '<i class="fas fa-check-circle text-success"></i> Hand complete!';
      return;
    }
    
    const action = this.actions[this.currentActionIndex];
    this.perform(action);
    this.currentActionIndex++;
    this.drawTable();
  }

  perform(action) {
    const actionEl = document.getElementById(`current-action-${this.index}`);
    const summary = document.getElementById(`summary-${this.index}`);

    let text = '';
    
    if (action.type === 'dealer') {
      text = `<i class="fas fa-chess-queen text-primary"></i> Dealer: Player ${action.value + 1}`;
      
    } else if (action.type === 'winner') {
      const playerIdx = action.player - 1;
      this.playerPots[playerIdx] += action.amount;
      this.pot = Math.max(0, this.pot - action.amount);
      this.playerStates[playerIdx] = 'winner';
      
      const isUser = action.player === this.gameData.user_player;
      text = `<i class="fas fa-trophy ${isUser ? 'text-warning' : 'text-success'}"></i> ` +
             `<strong>${isUser ? 'You' : 'Player ' + action.player} win${isUser ? '' : 's'} $${action.amount}!</strong>`;
      
    } else if (action.type === 'raw') {
      text = this.parseRaw(action.value);
    }

    actionEl.innerHTML = text;
    
    // Add to summary
    const summaryEl = document.getElementById(`summary-${this.index}`);
    if (summaryEl.innerHTML.includes('Actions will appear') || 
        summaryEl.innerHTML.includes('Reset')) {
      summaryEl.innerHTML = `<div>• ${text.replace(/<[^>]*>/g, '')}</div>`;
    } else {
      summaryEl.innerHTML += `<div>• ${text.replace(/<[^>]*>/g, '')}</div>`;
    }
    summaryEl.scrollTop = summaryEl.scrollHeight;
  }

  parseRaw(str) {
    // Community cards
    if (str.startsWith('com:')) {
      const cards = str.replace('com:', '').split(',');
      cards.forEach(card => {
        if (this.communityIndex < 5) {
          this.communityCards[this.communityIndex] = card.trim();
          this.communityIndex++;
        }
      });
      return `<i class="fas fa-layer-group text-info"></i> Community: ${cards.join(', ')}`;
    }

    // Player actions
    const [player, move] = str.split(':');
    if (!player || !player.startsWith('p')) return str;
    
    const playerNum = parseInt(player.replace('p', ''));
    const playerIdx = playerNum - 1;
    const isUser = playerNum === this.gameData.user_player;
    const playerLabel = isUser ? '<strong>You</strong>' : `Player ${playerNum}`;

    if (move === 'sb') {
      this.playerPots[playerIdx] -= this.gameData.small_blind;
      this.pot += this.gameData.small_blind;
      return `${playerLabel} post${isUser ? '' : 's'} <span class="badge bg-secondary">SB $${this.gameData.small_blind}</span>`;
      
    } else if (move === 'bb') {
      this.playerPots[playerIdx] -= this.gameData.big_blind;
      this.pot += this.gameData.big_blind;
      return `${playerLabel} post${isUser ? '' : 's'} <span class="badge bg-secondary">BB $${this.gameData.big_blind}</span>`;
      
    } else if (move.startsWith('c-')) {
      const amount = parseInt(move.split('-')[1]);
      this.playerPots[playerIdx] -= amount;
      this.pot += amount;
      return `${playerLabel} <span class="badge bg-primary">call${isUser ? '' : 's'} $${amount}</span>`;
      
    } else if (move.startsWith('r-')) {
      const amount = parseInt(move.split('-')[1]);
      this.playerPots[playerIdx] -= amount;
      this.pot += amount;
      return `${playerLabel} <span class="badge bg-danger">raise${isUser ? '' : 's'} $${amount}</span>`;
      
    } else if (move === 'F') {
      this.playerStates[playerIdx] = 'folded';
      return `${playerLabel} <span class="badge bg-dark">fold${isUser ? '' : 's'}</span>`;
      
    } else if (move.startsWith('a-')) {
      const amount = parseInt(move.split('-')[1]);
      this.playerPots[playerIdx] = 0;
      this.pot += amount;
      this.playerStates[playerIdx] = 'allin';
      return `${playerLabel} <span class="badge bg-warning text-dark">ALL-IN $${amount}</span>`;
      
    } else if (move.includes('-') && move.length <= 4) {
      // Card dealt
      const currentCards = this.playerHands[player] || 'XX-X : XX-X';
      if (currentCards === 'XX-X : XX-X') {
        this.playerHands[player] = `${move} : XX-X`;
      } else {
        const first = currentCards.split(' : ')[0];
        this.playerHands[player] = `${first} : ${move}`;
      }
      return `<i class="fas fa-clone"></i> ${playerLabel} receive${isUser ? '' : 's'} card`;
    }
    
    return str;
  }

  autoPlay() {
    const btn = document.getElementById(`auto-${this.index}`);
    
    if (this.interval) {
      clearInterval(this.interval);
      this.interval = null;
      btn.innerHTML = '<i class="fas fa-play"></i> Auto';
      return;
    }
    
    btn.innerHTML = '<i class="fas fa-pause"></i> Pause';
    this.interval = setInterval(() => {
      if (this.currentActionIndex >= this.actions.length) {
        clearInterval(this.interval);
        this.interval = null;
        btn.innerHTML = '<i class="fas fa-play"></i> Auto';
        return;
      }
      this.nextAction();
    }, 1000);
  }

  drawTable() {
    const ctx = this.ctx;
    ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    // Table background (green felt)
    ctx.fillStyle = '#0a5f3f';
    ctx.beginPath();
    ctx.ellipse(275, 200, 200, 120, 0, 0, 2 * Math.PI);
    ctx.fill();

    // Pot text
    ctx.fillStyle = '#ffffff';
    ctx.font = 'bold 18px Arial';
    ctx.textAlign = 'center';
    ctx.fillText(`POT: $${this.pot}`, 275, 150);

    // Community cards
    this.communityPositions.forEach((pos, i) => {
      this.drawCard(pos.x, pos.y, this.communityCards[i]);
    });

    // Players
    this.playerPositions.forEach((pos, i) => {
      const playerNum = i + 1;
      const isUser = playerNum === this.gameData.user_player;
      const state = this.playerStates[i];
      const cards = this.playerHands[`p${playerNum}`] || 'XX-X : XX-X';
      this.drawPlayer(pos.x, pos.y, playerNum, this.playerPots[i], cards, isUser, state);
    });
  }

  drawCard(x, y, text) {
    const ctx = this.ctx;
    const w = 50, h = 40;
    
    // Card background
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(x - w/2, y - h/2, w, h);
    ctx.strokeStyle = '#000000';
    ctx.lineWidth = 2;
    ctx.strokeRect(x - w/2, y - h/2, w, h);
    
    // Card text
    ctx.fillStyle = '#000000';
    ctx.font = '12px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(text, x, y);
  }

  drawPlayer(x, y, num, chips, cards, isUser, state) {
    const ctx = this.ctx;
    const w = 100, h = 70;
    
    // Determine colors based on state and user
    let fillColor = '#28a745';  // Green (active)
    let strokeColor = '#1e7e34';
    let strokeWidth = 2;
    
    if (state === 'winner') {
      fillColor = '#FFD700';  // Gold
      strokeColor = '#FFA500';
      strokeWidth = 4;
    } else if (state === 'folded') {
      fillColor = '#6c757d';  // Gray
      strokeColor = '#495057';
    } else if (state === 'allin') {
      fillColor = '#9d4edd';  // Purple
      strokeColor = '#7209b7';
      strokeWidth = 3;
    } else if (isUser) {
      fillColor = '#ffc107';  // Yellow/Amber for user
      strokeColor = '#ff9800';
      strokeWidth = 4;
    }

    // Player box
    ctx.fillStyle = fillColor;
    ctx.fillRect(x - w/2, y - h/2, w, h);
    ctx.strokeStyle = strokeColor;
    ctx.lineWidth = strokeWidth;
    ctx.strokeRect(x - w/2, y - h/2, w, h);

    // Player label
    ctx.fillStyle = state === 'folded' ? '#ffffff' : '#000000';
    ctx.font = isUser ? 'bold 13px Arial' : 'bold 11px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    const label = isUser ? `P${num} (YOU)` : `P${num}`;
    ctx.fillText(label, x, y - h/2 + 5);

    // Chips
    ctx.font = '12px Arial';
    ctx.fillText(`$${chips}`, x, y - h/2 + 25);

    // Cards - Always show all cards for all players
    ctx.font = '10px monospace';
    ctx.fillText(cards, x, y - h/2 + 45);
  }
}
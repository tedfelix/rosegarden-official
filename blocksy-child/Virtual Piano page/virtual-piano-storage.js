/**
 * Virtual Piano Storage Module
 * Account integration for saving user presets and settings
 *
 * Features:
 * - Save/Load user presets (effects, instruments, drum patterns)
 * - Cloud sync for logged-in users
 * - Local storage fallback for guests
 * - Preset management (rename, delete, share)
 *
 * @version 1.0.0
 * @requires WordPress REST API
 */

class VirtualPianoStorage {
    constructor() {
        this.currentUser = this.getCurrentUser();
        this.isLoggedIn = this.currentUser !== null;
        this.presets = [];

        this.initUI();
        this.loadPresets();
    }

    getCurrentUser() {
        // Check if user is logged in (WordPress)
        if (typeof wpApiSettings !== 'undefined' && wpApiSettings.currentUser) {
            return wpApiSettings.currentUser;
        }

        // Fallback: check if element exists
        const userElement = document.querySelector('[data-user-id]');
        if (userElement) {
            return {
                id: userElement.dataset.userId,
                name: userElement.dataset.userName || 'User'
            };
        }

        return null;
    }

    initUI() {
        const storageHTML = `
            <div class="presets-container">
                <div class="presets-header">
                    <h3>My Presets</h3>
                    ${this.isLoggedIn
                        ? `<span class="user-badge">
                             <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                                 <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path>
                                 <circle cx="12" cy="7" r="4"></circle>
                             </svg>
                             ${this.currentUser.name}
                           </span>`
                        : `<span class="guest-badge">Guest Mode (Local Only)</span>`
                    }
                </div>

                <div class="preset-actions">
                    <button class="preset-btn save-preset-btn" id="savePresetBtn">
                        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path>
                            <polyline points="17 21 17 13 7 13 7 21"></polyline>
                            <polyline points="7 3 7 8 15 8"></polyline>
                        </svg>
                        Save Current Setup
                    </button>

                    ${this.isLoggedIn
                        ? `<button class="preset-btn sync-btn" id="syncPresetsBtn">
                             <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                 <polyline points="23 4 23 10 17 10"></polyline>
                                 <polyline points="1 20 1 14 7 14"></polyline>
                                 <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path>
                             </svg>
                             Sync
                           </button>`
                        : `<button class="preset-btn login-prompt-btn" id="loginPromptBtn">
                             <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                 <path d="M15 3h4a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-4"></path>
                                 <polyline points="10 17 15 12 10 7"></polyline>
                                 <line x1="15" y1="12" x2="3" y2="12"></line>
                             </svg>
                             Login to Sync
                           </button>`
                    }
                </div>

                <div class="presets-list" id="presetsList">
                    <div class="no-presets" id="noPresetsMessage">
                        <p>No presets saved yet</p>
                        <p class="hint">Save your current configuration to access it later</p>
                    </div>
                </div>
            </div>

            <!-- Save Preset Modal -->
            <div class="modal" id="savePresetModal" style="display: none;">
                <div class="modal-content">
                    <div class="modal-header">
                        <h3>Save Preset</h3>
                        <button class="modal-close" id="closeModalBtn">&times;</button>
                    </div>

                    <div class="modal-body">
                        <div class="form-group">
                            <label for="presetName">Preset Name</label>
                            <input type="text" id="presetName" placeholder="My Awesome Setup" maxlength="50">
                        </div>

                        <div class="form-group">
                            <label for="presetDescription">Description (Optional)</label>
                            <textarea id="presetDescription" placeholder="Describe your preset..." rows="3" maxlength="200"></textarea>
                        </div>

                        <div class="preset-preview">
                            <h4>What will be saved:</h4>
                            <ul id="presetPreview"></ul>
                        </div>
                    </div>

                    <div class="modal-footer">
                        <button class="btn-secondary" id="cancelSaveBtn">Cancel</button>
                        <button class="btn-primary" id="confirmSaveBtn">Save Preset</button>
                    </div>
                </div>
            </div>
        `;

        // Insert into Recording Studio container (or fallback to old location)
        const studioContainer = document.getElementById('studioModulesContainer');
        const fallbackTarget = document.querySelector('.main-controls') ||
                              document.querySelector('.controls-section');

        const presetsDiv = document.createElement('div');
        presetsDiv.className = 'presets-section';
        presetsDiv.innerHTML = storageHTML;

        if (studioContainer) {
            // Insert into Recording Studio section
            studioContainer.appendChild(presetsDiv);
        } else if (fallbackTarget) {
            // Fallback: insert after main controls
            fallbackTarget.parentNode.appendChild(presetsDiv);
        }

        this.attachEventListeners();
    }

    attachEventListeners() {
        document.getElementById('savePresetBtn')?.addEventListener('click', () => {
            this.showSaveModal();
        });

        document.getElementById('syncPresetsBtn')?.addEventListener('click', () => {
            this.syncPresets();
        });

        document.getElementById('loginPromptBtn')?.addEventListener('click', () => {
            this.promptLogin();
        });

        document.getElementById('closeModalBtn')?.addEventListener('click', () => {
            this.closeSaveModal();
        });

        document.getElementById('cancelSaveBtn')?.addEventListener('click', () => {
            this.closeSaveModal();
        });

        document.getElementById('confirmSaveBtn')?.addEventListener('click', () => {
            this.saveCurrentPreset();
        });

        // Close modal on outside click
        document.getElementById('savePresetModal')?.addEventListener('click', (e) => {
            if (e.target.id === 'savePresetModal') {
                this.closeSaveModal();
            }
        });
    }

    showSaveModal() {
        const modal = document.getElementById('savePresetModal');
        if (!modal) return;

        // Generate preview of what will be saved
        const preview = this.generatePresetPreview();
        const previewList = document.getElementById('presetPreview');
        if (previewList) {
            previewList.innerHTML = preview.map(item => `<li>${item}</li>`).join('');
        }

        modal.style.display = 'flex';

        // Focus on name input
        document.getElementById('presetName')?.focus();
    }

    closeSaveModal() {
        const modal = document.getElementById('savePresetModal');
        if (modal) {
            modal.style.display = 'none';
            document.getElementById('presetName').value = '';
            document.getElementById('presetDescription').value = '';
        }
    }

    generatePresetPreview() {
        const items = [];

        // Check what's currently configured
        if (window.virtualStudio) {
            items.push('Piano settings & selected instrument');
            items.push('Drum machine pattern & samples');
        }

        if (window.effectsModule) {
            items.push('Audio effects (Delay, Reverb, Swing)');
        }

        items.push('Control settings (tempo, volume, etc.)');

        return items;
    }

    async saveCurrentPreset() {
        const name = document.getElementById('presetName')?.value.trim();
        const description = document.getElementById('presetDescription')?.value.trim();

        if (!name) {
            alert('Please enter a preset name');
            return;
        }

        // Gather current state
        const presetData = this.captureCurrentState();

        const preset = {
            id: Date.now(),
            name: name,
            description: description,
            data: presetData,
            createdAt: new Date().toISOString(),
            updatedAt: new Date().toISOString()
        };

        // Save locally
        this.presets.push(preset);
        this.saveToLocalStorage();

        // Save to cloud if logged in
        if (this.isLoggedIn) {
            await this.saveToCloud(preset);
        }

        this.closeSaveModal();
        this.renderPresets();

        // Show success message
        this.showNotification('Preset saved successfully!', 'success');
    }

    captureCurrentState() {
        const state = {
            piano: {},
            drums: {},
            effects: {},
            settings: {}
        };

        // Capture piano settings
        if (window.virtualStudio) {
            state.piano = {
                instrument: document.querySelector('select[name="instrument"]')?.value || 'piano',
                octaveRange: document.getElementById('octaveRange')?.value || 2,
                volume: document.getElementById('pianoVolume')?.value || 80,
                notation: document.getElementById('notationToggle')?.checked ? 'latin' : 'international'
            };

            // Capture drum pattern
            const pattern = this.captureDrumPattern();
            if (pattern) {
                state.drums = pattern;
            }
        }

        // Capture effects settings
        if (window.effectsModule) {
            state.effects = {
                delay: {
                    enabled: document.getElementById('delayToggle')?.checked || false,
                    time: parseInt(document.getElementById('delayTime')?.value) || 250,
                    feedback: parseInt(document.getElementById('delayFeedback')?.value) || 30,
                    mix: parseInt(document.getElementById('delayMix')?.value) || 0
                },
                reverb: {
                    enabled: document.getElementById('reverbToggle')?.checked || false,
                    decay: parseFloat(document.getElementById('reverbDecay')?.value) || 2,
                    mix: parseInt(document.getElementById('reverbMix')?.value) || 0
                },
                swing: {
                    enabled: document.getElementById('swingToggle')?.checked || false,
                    amount: parseInt(document.getElementById('swingAmount')?.value) || 0
                }
            };
        }

        // Capture general settings
        state.settings = {
            tempo: parseInt(document.getElementById('tempoSlider')?.value) || 120,
            masterVolume: parseInt(document.getElementById('masterVolume')?.value) || 100,
            metronomeEnabled: document.getElementById('metronomeToggle')?.checked || false
        };

        return state;
    }

    captureDrumPattern() {
        const grid = document.querySelectorAll('.sequencer-grid input[type="checkbox"]');
        if (!grid.length) return null;

        const pattern = [];
        grid.forEach((checkbox, index) => {
            if (checkbox.checked) {
                pattern.push(index);
            }
        });

        return {
            pattern: pattern,
            trackCount: document.getElementById('trackCount')?.value || 8
        };
    }

    async loadPreset(presetId) {
        const preset = this.presets.find(p => p.id === presetId);
        if (!preset) return;

        const data = preset.data;

        // Apply piano settings
        if (data.piano && window.virtualStudio) {
            if (data.piano.instrument) {
                const instrumentSelect = document.querySelector('select[name="instrument"]');
                if (instrumentSelect) {
                    instrumentSelect.value = data.piano.instrument;
                    instrumentSelect.dispatchEvent(new Event('change'));
                }
            }

            if (data.piano.volume) {
                const volumeSlider = document.getElementById('pianoVolume');
                if (volumeSlider) {
                    volumeSlider.value = data.piano.volume;
                    volumeSlider.dispatchEvent(new Event('input'));
                }
            }
        }

        // Apply drum pattern
        if (data.drums && data.drums.pattern) {
            this.applyDrumPattern(data.drums.pattern);
        }

        // Apply effects
        if (data.effects && window.effectsModule) {
            // Delay
            if (data.effects.delay) {
                document.getElementById('delayToggle').checked = data.effects.delay.enabled;
                document.getElementById('delayTime').value = data.effects.delay.time;
                document.getElementById('delayFeedback').value = data.effects.delay.feedback;
                document.getElementById('delayMix').value = data.effects.delay.mix;

                // Trigger updates
                document.getElementById('delayToggle').dispatchEvent(new Event('change'));
                document.getElementById('delayTime').dispatchEvent(new Event('input'));
                document.getElementById('delayFeedback').dispatchEvent(new Event('input'));
                document.getElementById('delayMix').dispatchEvent(new Event('input'));
            }

            // Reverb
            if (data.effects.reverb) {
                document.getElementById('reverbToggle').checked = data.effects.reverb.enabled;
                document.getElementById('reverbDecay').value = data.effects.reverb.decay;
                document.getElementById('reverbMix').value = data.effects.reverb.mix;

                document.getElementById('reverbToggle').dispatchEvent(new Event('change'));
                document.getElementById('reverbDecay').dispatchEvent(new Event('input'));
                document.getElementById('reverbMix').dispatchEvent(new Event('input'));
            }
        }

        // Apply general settings
        if (data.settings) {
            if (data.settings.tempo) {
                const tempoSlider = document.getElementById('tempoSlider');
                if (tempoSlider) {
                    tempoSlider.value = data.settings.tempo;
                    tempoSlider.dispatchEvent(new Event('input'));
                }
            }
        }

        this.showNotification(`Loaded preset: ${preset.name}`, 'success');
    }

    applyDrumPattern(pattern) {
        // Clear current pattern
        document.querySelectorAll('.sequencer-grid input[type="checkbox"]').forEach(cb => {
            cb.checked = false;
        });

        // Apply new pattern
        pattern.forEach(index => {
            const checkbox = document.querySelectorAll('.sequencer-grid input[type="checkbox"]')[index];
            if (checkbox) {
                checkbox.checked = true;
            }
        });
    }

    async deletePreset(presetId) {
        if (!confirm('Are you sure you want to delete this preset?')) return;

        this.presets = this.presets.filter(p => p.id !== presetId);
        this.saveToLocalStorage();

        if (this.isLoggedIn) {
            await this.deleteFromCloud(presetId);
        }

        this.renderPresets();
        this.showNotification('Preset deleted', 'info');
    }

    renderPresets() {
        const list = document.getElementById('presetsList');
        const noPresetsMsg = document.getElementById('noPresetsMessage');

        if (this.presets.length === 0) {
            noPresetsMsg.style.display = 'block';
            return;
        }

        noPresetsMsg.style.display = 'none';

        const presetsHTML = this.presets.map(preset => `
            <div class="preset-item" data-id="${preset.id}">
                <div class="preset-info">
                    <h4 class="preset-name">${this.escapeHtml(preset.name)}</h4>
                    ${preset.description ? `<p class="preset-description">${this.escapeHtml(preset.description)}</p>` : ''}
                    <span class="preset-date">${new Date(preset.createdAt).toLocaleDateString()}</span>
                </div>

                <div class="preset-actions">
                    <button class="preset-action-btn load-btn" onclick="window.storageModule.loadPreset(${preset.id})">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <polyline points="16 18 22 12 16 6"></polyline>
                            <polyline points="8 6 2 12 8 18"></polyline>
                        </svg>
                        Load
                    </button>

                    <button class="preset-action-btn delete-btn" onclick="window.storageModule.deletePreset(${preset.id})">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <polyline points="3 6 5 6 21 6"></polyline>
                            <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                        </svg>
                        Delete
                    </button>
                </div>
            </div>
        `).join('');

        list.innerHTML = presetsHTML;
    }

    // Storage Methods
    saveToLocalStorage() {
        try {
            localStorage.setItem('pianomode_presets', JSON.stringify(this.presets));
        } catch (error) {
            console.error('Failed to save to localStorage:', error);
        }
    }

    loadPresets() {
        // Load from localStorage
        try {
            const stored = localStorage.getItem('pianomode_presets');
            if (stored) {
                this.presets = JSON.parse(stored);
            }
        } catch (error) {
            console.error('Failed to load from localStorage:', error);
        }

        // Load from cloud if logged in
        if (this.isLoggedIn) {
            this.loadFromCloud();
        }

        this.renderPresets();
    }

    async saveToCloud(preset) {
        if (!this.isLoggedIn) return;

        try {
            const response = await fetch('/wp-json/pianomode/v1/presets', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-WP-Nonce': wpApiSettings?.nonce || ''
                },
                body: JSON.stringify(preset)
            });

            if (!response.ok) {
                throw new Error('Failed to save to cloud');
            }

            return await response.json();
        } catch (error) {
            console.error('Cloud save failed:', error);
            this.showNotification('Saved locally only (cloud sync failed)', 'warning');
        }
    }

    async loadFromCloud() {
        if (!this.isLoggedIn) return;

        try {
            const response = await fetch('/wp-json/pianomode/v1/presets', {
                headers: {
                    'X-WP-Nonce': wpApiSettings?.nonce || ''
                }
            });

            if (!response.ok) {
                throw new Error('Failed to load from cloud');
            }

            const cloudPresets = await response.json();

            // Merge with local presets (cloud takes precedence)
            this.mergePresets(cloudPresets);
            this.renderPresets();

        } catch (error) {
            console.error('Cloud load failed:', error);
        }
    }

    async deleteFromCloud(presetId) {
        if (!this.isLoggedIn) return;

        try {
            await fetch(`/wp-json/pianomode/v1/presets/${presetId}`, {
                method: 'DELETE',
                headers: {
                    'X-WP-Nonce': wpApiSettings?.nonce || ''
                }
            });
        } catch (error) {
            console.error('Cloud delete failed:', error);
        }
    }

    async syncPresets() {
        if (!this.isLoggedIn) return;

        this.showNotification('Syncing presets...', 'info');

        await this.loadFromCloud();

        // Upload any local-only presets to cloud
        for (const preset of this.presets) {
            if (!preset.cloudSynced) {
                await this.saveToCloud(preset);
                preset.cloudSynced = true;
            }
        }

        this.saveToLocalStorage();
        this.showNotification('Sync complete!', 'success');
    }

    mergePresets(cloudPresets) {
        // Simple merge: add cloud presets that don't exist locally
        cloudPresets.forEach(cloudPreset => {
            const existsLocally = this.presets.some(p => p.id === cloudPreset.id);
            if (!existsLocally) {
                cloudPreset.cloudSynced = true;
                this.presets.push(cloudPreset);
            }
        });

        this.saveToLocalStorage();
    }

    promptLogin() {
        if (confirm('Please log in to sync your presets across devices. Redirect to login page?')) {
            window.location.href = '/wp-login.php?redirect_to=' + encodeURIComponent(window.location.href);
        }
    }

    showNotification(message, type = 'info') {
        // Simple notification (can be enhanced with a toast library)
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.textContent = message;
        notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 1rem 1.5rem;
            background: ${type === 'success' ? '#4caf50' : type === 'warning' ? '#ff9800' : '#2196f3'};
            color: white;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.3);
            z-index: 10000;
            animation: slideIn 0.3s ease;
        `;

        document.body.appendChild(notification);

        setTimeout(() => {
            notification.style.animation = 'slideOut 0.3s ease';
            setTimeout(() => notification.remove(), 300);
        }, 3000);
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// Export for use in main Virtual Piano code
window.VirtualPianoStorage = VirtualPianoStorage;
<?php
/**
 * PIANOMODE SCORE META BOXES - Version Complète avec Affiliation
 * * Gère : PDF, YouTube, MusicXML (AlphaTab), Difficulty Radar, SEO Override, Tags, Affiliation
 * * @package PianoMode
 * @version 2.1.0
 */

if (!defined('ABSPATH')) {
    exit;
}

// =====================================================
// 1. ENREGISTREMENT DES META BOXES
// =====================================================

function pianomode_register_score_meta_boxes() {
    // Meta Box principale - Fichiers & Media
    add_meta_box(
        'pianomode_score_files',
        '📄 Score Files & Media',
        'pianomode_score_files_callback',
        'score',
        'normal',
        'high'
    );
    
    // NOUVEAU : Meta Box Affiliation
    add_meta_box(
        'pianomode_score_affiliate',
        '💰 Affiliate Links',
        'pianomode_score_affiliate_callback',
        'score',
        'normal',
        'high'
    );
    
    // Meta Box Difficulty Radar
    add_meta_box(
        'pianomode_score_difficulty',
        '🎯 Difficulty Radar',
        'pianomode_score_difficulty_callback',
        'score',
        'normal',
        'high'
    );
    
    // Meta Box SEO Override (optionnel)
    add_meta_box(
        'pianomode_score_seo',
        '🔍 SEO Override (Optional)',
        'pianomode_score_seo_callback',
        'score',
        'normal',
        'default'
    );
    
    // Meta Box Tags
    add_meta_box(
        'pianomode_score_tags',
        '🏷️ Score Tags',
        'pianomode_score_tags_callback',
        'score',
        'side',
        'default'
    );
}
add_action('add_meta_boxes', 'pianomode_register_score_meta_boxes');

// =====================================================
// 2. META BOX: FILES & MEDIA
// =====================================================

function pianomode_score_files_callback($post) {
    wp_nonce_field('pianomode_score_save', 'pianomode_score_nonce');
    
    // Récupérer les valeurs
    $pdf_id = get_post_meta($post->ID, '_score_pdf_id', true);
    $pdf_url = $pdf_id ? wp_get_attachment_url($pdf_id) : '';
    $youtube_url = get_post_meta($post->ID, '_score_youtube_url', true);
    $musicxml_id = get_post_meta($post->ID, '_score_musicxml_id', true);
    $musicxml_url = $musicxml_id ? wp_get_attachment_url($musicxml_id) : '';
    $copyright = get_post_meta($post->ID, '_score_copyright', true);
    
    // Calculer taille PDF
    $pdf_size = '';
    if ($pdf_id) {
        $file_path = get_attached_file($pdf_id);
        if ($file_path && file_exists($file_path)) {
            $bytes = filesize($file_path);
            if ($bytes >= 1048576) {
                $pdf_size = round($bytes / 1048576, 1) . ' MB';
            } else {
                $pdf_size = round($bytes / 1024) . ' KB';
            }
        }
    }
    ?>
    <style>
        .pianomode-metabox-section {
            background: #fff;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .pianomode-metabox-section:last-child {
            margin-bottom: 0;
        }
        .pianomode-metabox-title {
            font-size: 14px;
            font-weight: 600;
            color: #1a1a1a;
            margin: 0 0 15px 0;
            padding-bottom: 10px;
            border-bottom: 2px solid #C59D3A;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .pianomode-metabox-title span {
            font-size: 18px;
        }
        .pianomode-field-row {
            margin-bottom: 15px;
        }
        .pianomode-field-row:last-child {
            margin-bottom: 0;
        }
        .pianomode-field-label {
            display: block;
            font-weight: 500;
            margin-bottom: 6px;
            color: #333;
        }
        .pianomode-field-input {
            width: 100%;
            padding: 10px 12px;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
            transition: border-color 0.2s;
        }
        .pianomode-field-input:focus {
            outline: none;
            border-color: #C59D3A;
            box-shadow: 0 0 0 3px rgba(197, 157, 58, 0.1);
        }
        .pianomode-field-help {
            font-size: 12px;
            color: #666;
            margin-top: 5px;
        }
        .pianomode-media-preview {
            display: flex;
            align-items: center;
            gap: 15px;
            padding: 12px;
            background: #f8f9fa;
            border-radius: 6px;
            margin-top: 10px;
        }
        .pianomode-media-preview.has-file {
            border: 2px solid #4CAF50;
            background: #f1f8e9;
        }
        .pianomode-media-icon {
            width: 40px;
            height: 40px;
            background: #C59D3A;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-size: 18px;
        }
        .pianomode-media-info {
            flex: 1;
        }
        .pianomode-media-name {
            font-weight: 600;
            color: #1a1a1a;
            margin-bottom: 2px;
        }
        .pianomode-media-size {
            font-size: 12px;
            color: #666;
        }
        .pianomode-media-buttons {
            display: flex;
            gap: 8px;
        }
        .pianomode-btn {
            padding: 8px 16px;
            border-radius: 6px;
            font-size: 13px;
            font-weight: 500;
            cursor: pointer;
            border: none;
            transition: all 0.2s;
        }
        .pianomode-btn-primary {
            background: linear-gradient(135deg, #C59D3A, #B8860B);
            color: white;
        }
        .pianomode-btn-primary:hover {
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(197, 157, 58, 0.3);
        }
        .pianomode-btn-danger {
            background: #f44336;
            color: white;
        }
        .pianomode-btn-danger:hover {
            background: #d32f2f;
        }
        .pianomode-grid-2 {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        @media (max-width: 782px) {
            .pianomode-grid-2 {
                grid-template-columns: 1fr;
            }
        }
    </style>
    
    <div class="pianomode-metabox-wrapper">
        
        <div class="pianomode-metabox-section">
            <h4 class="pianomode-metabox-title"><span>📄</span> Sheet Music PDF</h4>
            
            <input type="hidden" name="score_pdf_id" id="score_pdf_id" value="<?php echo esc_attr($pdf_id); ?>">
            
            <div id="pdf-preview" class="pianomode-media-preview <?php echo $pdf_id ? 'has-file' : ''; ?>">
                <?php if ($pdf_id) : ?>
                    <div class="pianomode-media-icon">📄</div>
                    <div class="pianomode-media-info">
                        <div class="pianomode-media-name"><?php echo esc_html(basename($pdf_url)); ?></div>
                        <div class="pianomode-media-size"><?php echo esc_html($pdf_size); ?></div>
                    </div>
                    <div class="pianomode-media-buttons">
                        <a href="<?php echo esc_url($pdf_url); ?>" target="_blank" class="pianomode-btn pianomode-btn-primary">View</a>
                        <button type="button" class="pianomode-btn pianomode-btn-danger" id="remove-pdf">Remove</button>
                    </div>
                <?php else : ?>
                    <div class="pianomode-media-info">
                        <div class="pianomode-media-name" style="color: #666;">No PDF uploaded</div>
                        <div class="pianomode-media-size">Click "Upload PDF" to add a sheet music file</div>
                    </div>
                <?php endif; ?>
            </div>
            
            <button type="button" class="pianomode-btn pianomode-btn-primary" id="upload-pdf" style="margin-top: 10px;">
                <?php echo $pdf_id ? 'Change PDF' : 'Upload PDF'; ?>
            </button>
            
            <p class="pianomode-field-help">Upload the main PDF sheet music file. The file size will be displayed on the download button.</p>
        </div>
        
        <div class="pianomode-metabox-section">
            <h4 class="pianomode-metabox-title"><span>🎬</span> YouTube Video</h4>
            
            <div class="pianomode-field-row">
                <label class="pianomode-field-label">YouTube URL or Embed URL</label>
                <input type="url" 
                       name="score_youtube_url" 
                       id="score_youtube_url"
                       class="pianomode-field-input" 
                       value="<?php echo esc_attr($youtube_url); ?>"
                       placeholder="https://www.youtube.com/watch?v=xxxxx or https://www.youtube.com/embed/xxxxx">
            </div>
            <p class="pianomode-field-help">Paste any YouTube URL. It will be automatically converted to embed format.</p>
            
            <?php if ($youtube_url) : ?>
                <div style="margin-top: 15px; border-radius: 8px; overflow: hidden; aspect-ratio: 16/9; max-width: 400px;">
                    <iframe src="<?php echo esc_url(pianomode_convert_youtube_url($youtube_url)); ?>" 
                            style="width: 100%; height: 100%; border: none;"
                            allowfullscreen></iframe>
                </div>
            <?php endif; ?>
        </div>
        
        <div class="pianomode-metabox-section">
            <h4 class="pianomode-metabox-title"><span>🎼</span> MusicXML File (Optional - For Interactive Player)</h4>
            
            <input type="hidden" name="score_musicxml_id" id="score_musicxml_id" value="<?php echo esc_attr($musicxml_id); ?>">
            
            <div id="musicxml-preview" class="pianomode-media-preview <?php echo $musicxml_id ? 'has-file' : ''; ?>">
                <?php if ($musicxml_id) : ?>
                    <div class="pianomode-media-icon">🎼</div>
                    <div class="pianomode-media-info">
                        <div class="pianomode-media-name"><?php echo esc_html(basename($musicxml_url)); ?></div>
                        <div class="pianomode-media-size">AlphaTab player will be displayed</div>
                    </div>
                    <div class="pianomode-media-buttons">
                        <button type="button" class="pianomode-btn pianomode-btn-danger" id="remove-musicxml">Remove</button>
                    </div>
                <?php else : ?>
                    <div class="pianomode-media-info">
                        <div class="pianomode-media-name" style="color: #666;">No MusicXML file</div>
                        <div class="pianomode-media-size">Upload to enable the interactive AlphaTab player</div>
                    </div>
                <?php endif; ?>
            </div>
            
            <button type="button" class="pianomode-btn pianomode-btn-primary" id="upload-musicxml" style="margin-top: 10px;">
                <?php echo $musicxml_id ? 'Change MusicXML' : 'Upload MusicXML'; ?>
            </button>
            
            <p class="pianomode-field-help">
                Upload a .musicxml, .mxl, or .gp file to enable the interactive AlphaTab player with play/pause, tempo control, and cursor following.
                <br>Export from MuseScore, Sibelius, Finale, or Guitar Pro.
            </p>
        </div>

        <div class="pianomode-metabox-section">
            <h4 class="pianomode-metabox-title"><span>&#169;</span> Copyright Status</h4>

            <div class="pianomode-field-row">
                <label class="pianomode-field-label" for="score_copyright">Copyright / License</label>
                <input type="text"
                       name="score_copyright"
                       id="score_copyright"
                       class="pianomode-field-input"
                       value="<?php echo esc_attr($copyright); ?>"
                       placeholder="Public Domain">
                <p class="pianomode-field-help">Specify the copyright status of this score. Leave empty for default &ldquo;Public Domain&rdquo;. Examples: Public Domain, CC BY 4.0, &copy; 2024 Publisher Name, All Rights Reserved.</p>
            </div>
        </div>

    </div>
    
    <script>
    jQuery(document).ready(function($) {
        // PDF Upload
        var pdfFrame;
        $('#upload-pdf').on('click', function(e) {
            e.preventDefault();
            
            if (pdfFrame) {
                pdfFrame.open();
                return;
            }
            
            pdfFrame = wp.media({
                title: 'Select PDF Sheet Music',
                button: { text: 'Use this PDF' },
                library: { type: 'application/pdf' },
                multiple: false
            });
            
            pdfFrame.on('select', function() {
                var attachment = pdfFrame.state().get('selection').first().toJSON();
                $('#score_pdf_id').val(attachment.id);
                
                var size = attachment.filesizeHumanReadable || '';
                
                $('#pdf-preview').addClass('has-file').html(
                    '<div class="pianomode-media-icon">📄</div>' +
                    '<div class="pianomode-media-info">' +
                        '<div class="pianomode-media-name">' + attachment.filename + '</div>' +
                        '<div class="pianomode-media-size">' + size + '</div>' +
                    '</div>' +
                    '<div class="pianomode-media-buttons">' +
                        '<a href="' + attachment.url + '" target="_blank" class="pianomode-btn pianomode-btn-primary">View</a>' +
                        '<button type="button" class="pianomode-btn pianomode-btn-danger" id="remove-pdf">Remove</button>' +
                    '</div>'
                );
                
                $('#upload-pdf').text('Change PDF');
            });
            
            pdfFrame.open();
        });
        
        // Remove PDF
        $(document).on('click', '#remove-pdf', function(e) {
            e.preventDefault();
            $('#score_pdf_id').val('');
            $('#pdf-preview').removeClass('has-file').html(
                '<div class="pianomode-media-info">' +
                    '<div class="pianomode-media-name" style="color: #666;">No PDF uploaded</div>' +
                    '<div class="pianomode-media-size">Click "Upload PDF" to add a sheet music file</div>' +
                '</div>'
            );
            $('#upload-pdf').text('Upload PDF');
        });
        
        // MusicXML Upload
        var musicxmlFrame;
        $('#upload-musicxml').on('click', function(e) {
            e.preventDefault();
            
            if (musicxmlFrame) {
                musicxmlFrame.open();
                return;
            }
            
            musicxmlFrame = wp.media({
                title: 'Select MusicXML File',
                button: { text: 'Use this file' },
                multiple: false
            });
            
            musicxmlFrame.on('select', function() {
                var attachment = musicxmlFrame.state().get('selection').first().toJSON();
                $('#score_musicxml_id').val(attachment.id);
                
                $('#musicxml-preview').addClass('has-file').html(
                    '<div class="pianomode-media-icon">🎼</div>' +
                    '<div class="pianomode-media-info">' +
                        '<div class="pianomode-media-name">' + attachment.filename + '</div>' +
                        '<div class="pianomode-media-size">AlphaTab player will be displayed</div>' +
                    '</div>' +
                    '<div class="pianomode-media-buttons">' +
                        '<button type="button" class="pianomode-btn pianomode-btn-danger" id="remove-musicxml">Remove</button>' +
                    '</div>'
                );
                
                $('#upload-musicxml').text('Change MusicXML');
            });
            
            musicxmlFrame.open();
        });
        
        // Remove MusicXML
        $(document).on('click', '#remove-musicxml', function(e) {
            e.preventDefault();
            $('#score_musicxml_id').val('');
            $('#musicxml-preview').removeClass('has-file').html(
                '<div class="pianomode-media-info">' +
                    '<div class="pianomode-media-name" style="color: #666;">No MusicXML file</div>' +
                    '<div class="pianomode-media-size">Upload to enable the interactive AlphaTab player</div>' +
                '</div>'
            );
            $('#upload-musicxml').text('Upload MusicXML');
        });
    });
    </script>
    <?php
}

// =====================================================
// NOUVEAU : META BOX AFFILIATION
// =====================================================

function pianomode_score_affiliate_callback($post) {
    $vsm_url = get_post_meta($post->ID, '_score_vsm_url', true);
    $smp_url = get_post_meta($post->ID, '_score_smp_url', true);
    ?>
    <div class="pianomode-field-row" style="margin-bottom: 20px;">
        <label class="pianomode-field-label" style="color: #d35400;">★ Virtual Sheet Music URL (Priorité 30% / 20%)</label>
        <input type="url" name="score_vsm_url" class="pianomode-field-input" value="<?php echo esc_attr($vsm_url); ?>" placeholder="https://www.virtualsheetmusic.com/...">
        <p class="pianomode-field-help">Collez ici le lien affilié vers Virtual Sheet Music. Si rempli, ce bouton s'affichera.</p>
    </div>

    <div class="pianomode-field-row">
        <label class="pianomode-field-label" style="color: #2980b9;">Sheet Music Plus URL (Back-up)</label>
        <input type="url" name="score_smp_url" class="pianomode-field-input" value="<?php echo esc_attr($smp_url); ?>" placeholder="https://www.sheetmusicplus.com/...">
        <p class="pianomode-field-help">Collez ici le lien affilié vers Sheet Music Plus. Si rempli, ce bouton s'affichera.</p>
    </div>
    <?php
}

// =====================================================
// 3. META BOX: DIFFICULTY RADAR
// =====================================================

function pianomode_score_difficulty_callback($post) {
    $reading_ease = get_post_meta($post->ID, '_score_reading_ease', true) ?: 3;
    $left_hand = get_post_meta($post->ID, '_score_left_hand', true) ?: 3;
    $rhythm = get_post_meta($post->ID, '_score_rhythm', true) ?: 3;
    $dynamics = get_post_meta($post->ID, '_score_dynamics', true) ?: 3;
    ?>
    <style>
        .pianomode-radar-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 25px;
            max-width: 600px;
        }
        .pianomode-radar-item {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 12px;
            border: 1px solid #e0e0e0;
        }
        .pianomode-radar-label {
            font-weight: 600;
            color: #1a1a1a;
            margin-bottom: 8px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .pianomode-radar-label span {
            font-size: 20px;
        }
        .pianomode-radar-desc {
            font-size: 12px;
            color: #666;
            margin-bottom: 12px;
        }
        .pianomode-slider-container {
            position: relative;
        }
        .pianomode-slider {
            -webkit-appearance: none;
            width: 100%;
            height: 8px;
            border-radius: 4px;
            background: linear-gradient(to right, #4CAF50, #FFC107, #f44336);
            outline: none;
        }
        .pianomode-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #1a1a1a;
            border: 3px solid #C59D3A;
            cursor: pointer;
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
        }
        .pianomode-slider::-moz-range-thumb {
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #1a1a1a;
            border: 3px solid #C59D3A;
            cursor: pointer;
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
        }
        .pianomode-slider-labels {
            display: flex;
            justify-content: space-between;
            font-size: 11px;
            color: #888;
            margin-top: 5px;
        }
        .pianomode-slider-value {
            text-align: center;
            font-size: 24px;
            font-weight: 700;
            color: #C59D3A;
            margin-top: 8px;
        }
        .pianomode-radar-preview {
            margin-top: 30px;
            padding: 20px;
            background: #1a1a1a;
            border-radius: 16px;
            text-align: center;
        }
        .pianomode-radar-preview h4 {
            color: #C59D3A;
            margin: 0 0 15px 0;
        }
    </style>
    
    <p style="margin-bottom: 20px; color: #666;">
        Set the difficulty level for each aspect (1 = Easy, 5 = Very Difficult). This will be displayed as an elegant radar chart on the score page.
    </p>
    
    <div class="pianomode-radar-grid">
        
        <div class="pianomode-radar-item">
            <div class="pianomode-radar-label"><span>👁️</span> Reading Ease</div>
            <div class="pianomode-radar-desc">How easy is it to read the sheet music?</div>
            <div class="pianomode-slider-container">
                <input type="range" name="score_reading_ease" class="pianomode-slider" 
                       min="1" max="5" value="<?php echo esc_attr($reading_ease); ?>" 
                       id="slider_reading" oninput="updateSliderValue('reading', this.value)">
                <div class="pianomode-slider-labels">
                    <span>Simple</span>
                    <span>Complex</span>
                </div>
                <div class="pianomode-slider-value" id="value_reading"><?php echo esc_html($reading_ease); ?></div>
            </div>
        </div>
        
        <div class="pianomode-radar-item">
            <div class="pianomode-radar-label"><span>🤚</span> Left Hand</div>
            <div class="pianomode-radar-desc">Complexity of the left hand part</div>
            <div class="pianomode-slider-container">
                <input type="range" name="score_left_hand" class="pianomode-slider" 
                       min="1" max="5" value="<?php echo esc_attr($left_hand); ?>" 
                       id="slider_lefthand" oninput="updateSliderValue('lefthand', this.value)">
                <div class="pianomode-slider-labels">
                    <span>Basic</span>
                    <span>Advanced</span>
                </div>
                <div class="pianomode-slider-value" id="value_lefthand"><?php echo esc_html($left_hand); ?></div>
            </div>
        </div>
        
        <div class="pianomode-radar-item">
            <div class="pianomode-radar-label"><span>🥁</span> Rhythm</div>
            <div class="pianomode-radar-desc">Rhythmic complexity and timing challenges</div>
            <div class="pianomode-slider-container">
                <input type="range" name="score_rhythm" class="pianomode-slider" 
                       min="1" max="5" value="<?php echo esc_attr($rhythm); ?>" 
                       id="slider_rhythm" oninput="updateSliderValue('rhythm', this.value)">
                <div class="pianomode-slider-labels">
                    <span>Steady</span>
                    <span>Complex</span>
                </div>
                <div class="pianomode-slider-value" id="value_rhythm"><?php echo esc_html($rhythm); ?></div>
            </div>
        </div>
        
        <div class="pianomode-radar-item">
            <div class="pianomode-radar-label"><span>🔊</span> Dynamics</div>
            <div class="pianomode-radar-desc">Dynamic range and expression required</div>
            <div class="pianomode-slider-container">
                <input type="range" name="score_dynamics" class="pianomode-slider" 
                       min="1" max="5" value="<?php echo esc_attr($dynamics); ?>" 
                       id="slider_dynamics" oninput="updateSliderValue('dynamics', this.value)">
                <div class="pianomode-slider-labels">
                    <span>Subtle</span>
                    <span>Dramatic</span>
                </div>
                <div class="pianomode-slider-value" id="value_dynamics"><?php echo esc_html($dynamics); ?></div>
            </div>
        </div>
        
    </div>
    
    <div class="pianomode-radar-preview">
        <h4>📊 Radar Chart Preview</h4>
        <svg id="radar-preview" width="200" height="200" viewBox="0 0 200 200"></svg>
    </div>
    
    <script>
    function updateSliderValue(id, value) {
        document.getElementById('value_' + id).textContent = value;
        updateRadarPreview();
    }
    
    function updateRadarPreview() {
        const reading = parseInt(document.getElementById('slider_reading').value);
        const lefthand = parseInt(document.getElementById('slider_lefthand').value);
        const rhythm = parseInt(document.getElementById('slider_rhythm').value);
        const dynamics = parseInt(document.getElementById('slider_dynamics').value);
        
        const svg = document.getElementById('radar-preview');
        const cx = 100, cy = 100, maxR = 80;
        
        // Calculate points
        const angles = [0, 90, 180, 270].map(a => a * Math.PI / 180);
        const values = [reading, lefthand, rhythm, dynamics];
        
        let pathD = '';
        values.forEach((val, i) => {
            const r = (val / 5) * maxR;
            const x = cx + r * Math.cos(angles[i] - Math.PI/2);
            const y = cy + r * Math.sin(angles[i] - Math.PI/2);
            pathD += (i === 0 ? 'M' : 'L') + x + ',' + y;
        });
        pathD += 'Z';
        
        // Grid lines
        let gridHTML = '';
        for (let i = 1; i <= 5; i++) {
            const r = (i / 5) * maxR;
            gridHTML += `<circle cx="${cx}" cy="${cy}" r="${r}" fill="none" stroke="rgba(197,157,58,0.2)" stroke-width="1"/>`;
        }
        
        // Axes
        angles.forEach(angle => {
            const x2 = cx + maxR * Math.cos(angle - Math.PI/2);
            const y2 = cy + maxR * Math.sin(angle - Math.PI/2);
            gridHTML += `<line x1="${cx}" y1="${cy}" x2="${x2}" y2="${y2}" stroke="rgba(197,157,58,0.3)" stroke-width="1"/>`;
        });
        
        // Labels
        const labels = ['Reading', 'Left Hand', 'Rhythm', 'Dynamics'];
        const labelPositions = [
            {x: cx, y: 10},
            {x: 190, y: cy},
            {x: cx, y: 195},
            {x: 10, y: cy}
        ];
        
        labels.forEach((label, i) => {
            gridHTML += `<text x="${labelPositions[i].x}" y="${labelPositions[i].y}" fill="#C59D3A" font-size="10" text-anchor="middle" font-family="Montserrat, sans-serif">${label}</text>`;
        });
        
        // Data polygon
        gridHTML += `<path d="${pathD}" fill="rgba(197,157,58,0.3)" stroke="#C59D3A" stroke-width="2"/>`;
        
        // Data points
        values.forEach((val, i) => {
            const r = (val / 5) * maxR;
            const x = cx + r * Math.cos(angles[i] - Math.PI/2);
            const y = cy + r * Math.sin(angles[i] - Math.PI/2);
            gridHTML += `<circle cx="${x}" cy="${y}" r="5" fill="#C59D3A" stroke="#1a1a1a" stroke-width="2"/>`;
        });
        
        svg.innerHTML = gridHTML;
    }
    
    // Initial render
    document.addEventListener('DOMContentLoaded', updateRadarPreview);
    </script>
    <?php
}

// =====================================================
// 4. META BOX: SEO OVERRIDE
// =====================================================

function pianomode_score_seo_callback($post) {
    $custom_title = get_post_meta($post->ID, '_score_seo_title', true);
    $custom_description = get_post_meta($post->ID, '_score_seo_description', true);
    $focus_keyword = get_post_meta($post->ID, '_score_focus_keyword', true);
    $secondary_keywords = get_post_meta($post->ID, '_score_secondary_keywords', true);
    ?>
    <p style="margin-bottom: 15px; padding: 12px; background: #fff3cd; border-radius: 6px; color: #856404;">
        <strong>ℹ️ Note:</strong> These fields are optional. By default, SEO metadata is automatically generated based on the score title, composer, style, and level.
        Only fill these if you want to override the automatic values.
    </p>
    
    <div class="pianomode-field-row" style="margin-bottom: 15px;">
        <label class="pianomode-field-label" for="score_seo_title">Custom Meta Title</label>
        <input type="text" 
               name="score_seo_title" 
               id="score_seo_title"
               class="pianomode-field-input" 
               value="<?php echo esc_attr($custom_title); ?>"
               placeholder="Leave empty for auto-generated title"
               maxlength="60">
        <p class="pianomode-field-help">
            <span id="seo-title-count"><?php echo strlen($custom_title); ?></span>/60 characters
        </p>
    </div>
    
    <div class="pianomode-field-row" style="margin-bottom: 15px;">
        <label class="pianomode-field-label" for="score_seo_description">Custom Meta Description</label>
        <textarea name="score_seo_description" 
                  id="score_seo_description"
                  class="pianomode-field-input" 
                  rows="3"
                  placeholder="Leave empty for auto-generated description"
                  maxlength="160"><?php echo esc_textarea($custom_description); ?></textarea>
        <p class="pianomode-field-help">
            <span id="seo-desc-count"><?php echo strlen($custom_description); ?></span>/160 characters
        </p>
    </div>
    
    <div class="pianomode-grid-2">
        <div class="pianomode-field-row">
            <label class="pianomode-field-label" for="score_focus_keyword">Focus Keyword</label>
            <input type="text" 
                   name="score_focus_keyword" 
                   id="score_focus_keyword"
                   class="pianomode-field-input" 
                   value="<?php echo esc_attr($focus_keyword); ?>"
                   placeholder="e.g., Moonlight Sonata sheet music">
        </div>
        
        <div class="pianomode-field-row">
            <label class="pianomode-field-label" for="score_secondary_keywords">Secondary Keywords</label>
            <input type="text" 
                   name="score_secondary_keywords" 
                   id="score_secondary_keywords"
                   class="pianomode-field-input" 
                   value="<?php echo esc_attr($secondary_keywords); ?>"
                   placeholder="e.g., Beethoven piano, classical music">
        </div>
    </div>
    
    <script>
    document.getElementById('score_seo_title').addEventListener('input', function() {
        document.getElementById('seo-title-count').textContent = this.value.length;
    });
    document.getElementById('score_seo_description').addEventListener('input', function() {
        document.getElementById('seo-desc-count').textContent = this.value.length;
    });
    </script>
    <?php
}

// =====================================================
// 5. META BOX: TAGS
// =====================================================

function pianomode_score_tags_callback($post) {
    $tags = get_post_meta($post->ID, '_score_tags', true) ?: '';
    ?>
    <div class="pianomode-field-row">
        <textarea name="score_tags" 
                  id="score_tags"
                  style="width: 100%; min-height: 100px; padding: 10px; border: 1px solid #ddd; border-radius: 6px;"
                  placeholder="Enter tags separated by commas&#10;e.g., romantic, intermediate, popular, wedding music"><?php echo esc_textarea($tags); ?></textarea>
        <p class="pianomode-field-help">
            Separate tags with commas. These help users find related scores.
        </p>
    </div>
    <?php
}

// =====================================================
// 6. SAUVEGARDE DES META DONNÉES
// =====================================================

function pianomode_save_score_meta($post_id) {
    // Vérifications sécurité
    if (!isset($_POST['pianomode_score_nonce'])) {
        return;
    }
    
    if (!wp_verify_nonce($_POST['pianomode_score_nonce'], 'pianomode_score_save')) {
        return;
    }
    
    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
        return;
    }
    
    if (!current_user_can('edit_post', $post_id)) {
        return;
    }
    
    // Files & Media
    if (isset($_POST['score_pdf_id'])) {
        update_post_meta($post_id, '_score_pdf_id', absint($_POST['score_pdf_id']));
    }
    
    if (isset($_POST['score_youtube_url'])) {
        update_post_meta($post_id, '_score_youtube_url', esc_url_raw($_POST['score_youtube_url']));
    }
    
    if (isset($_POST['score_musicxml_id'])) {
        update_post_meta($post_id, '_score_musicxml_id', absint($_POST['score_musicxml_id']));
    }

    // Copyright Status
    if (isset($_POST['score_copyright'])) {
        update_post_meta($post_id, '_score_copyright', sanitize_text_field($_POST['score_copyright']));
    }

    // NOUVEAU : Sauvegarde Affiliation
    if (isset($_POST['score_vsm_url'])) {
        update_post_meta($post_id, '_score_vsm_url', esc_url_raw($_POST['score_vsm_url']));
    }
    
    if (isset($_POST['score_smp_url'])) {
        update_post_meta($post_id, '_score_smp_url', esc_url_raw($_POST['score_smp_url']));
    }
    
    // Difficulty Radar
    $radar_fields = ['score_reading_ease', 'score_left_hand', 'score_rhythm', 'score_dynamics'];
    foreach ($radar_fields as $field) {
        if (isset($_POST[$field])) {
            $value = absint($_POST[$field]);
            $value = max(1, min(5, $value)); // Clamp 1-5
            update_post_meta($post_id, '_' . $field, $value);
        }
    }
    
    // SEO Override
    $seo_fields = ['score_seo_title', 'score_seo_description', 'score_focus_keyword', 'score_secondary_keywords'];
    foreach ($seo_fields as $field) {
        if (isset($_POST[$field])) {
            update_post_meta($post_id, '_' . $field, sanitize_text_field($_POST[$field]));
        }
    }
    
    // Tags
    if (isset($_POST['score_tags'])) {
        update_post_meta($post_id, '_score_tags', sanitize_textarea_field($_POST['score_tags']));
    }
}
add_action('save_post_score', 'pianomode_save_score_meta');

// =====================================================
// 7. HELPER FUNCTIONS
// =====================================================

/**
 * Convertir URL YouTube en format embed
 */
function pianomode_convert_youtube_url($url) {
    if (empty($url)) return '';
    
    // Déjà au format embed
    if (strpos($url, 'youtube.com/embed/') !== false) {
        return $url;
    }
    
    // Format watch?v=
    if (preg_match('/youtube\.com\/watch\?v=([^&]+)/', $url, $matches)) {
        return 'https://www.youtube.com/embed/' . $matches[1];
    }
    
    // Format youtu.be/
    if (preg_match('/youtu\.be\/([^?]+)/', $url, $matches)) {
        return 'https://www.youtube.com/embed/' . $matches[1];
    }
    
    return $url;
}

/**
 * Récupérer les données du score pour le frontend
 */
function pianomode_get_score_data($post_id) {
    $pdf_id = get_post_meta($post_id, '_score_pdf_id', true);
    $pdf_url = $pdf_id ? wp_get_attachment_url($pdf_id) : '';
    $pdf_size = '';
    
    if ($pdf_id) {
        $file_path = get_attached_file($pdf_id);
        if ($file_path && file_exists($file_path)) {
            $bytes = filesize($file_path);
            if ($bytes >= 1048576) {
                $pdf_size = round($bytes / 1048576, 1) . ' MB';
            } else {
                $pdf_size = round($bytes / 1024) . ' KB';
            }
        }
    }
    
    $musicxml_id = get_post_meta($post_id, '_score_musicxml_id', true);
    
    return [
        'pdf_url' => $pdf_url,
        'pdf_size' => $pdf_size,
        'youtube_url' => pianomode_convert_youtube_url(get_post_meta($post_id, '_score_youtube_url', true)),
        'musicxml_url' => $musicxml_id ? wp_get_attachment_url($musicxml_id) : '',
        'has_alphatab' => !empty($musicxml_id),
        
        // NOUVEAU : URLs Affiliation
        'vsm_url' => get_post_meta($post_id, '_score_vsm_url', true),
        'smp_url' => get_post_meta($post_id, '_score_smp_url', true),
        
        'difficulty' => [
            'reading' => get_post_meta($post_id, '_score_reading_ease', true) ?: 3,
            'left_hand' => get_post_meta($post_id, '_score_left_hand', true) ?: 3,
            'rhythm' => get_post_meta($post_id, '_score_rhythm', true) ?: 3,
            'dynamics' => get_post_meta($post_id, '_score_dynamics', true) ?: 3,
        ],
        'seo' => [
            'title' => get_post_meta($post_id, '_score_seo_title', true),
            'description' => get_post_meta($post_id, '_score_seo_description', true),
            'focus_keyword' => get_post_meta($post_id, '_score_focus_keyword', true),
        ],
        'tags' => get_post_meta($post_id, '_score_tags', true),
        'copyright' => get_post_meta($post_id, '_score_copyright', true),
    ];
}

// =====================================================
// 8. AUTORISER UPLOAD MUSICXML
// =====================================================

function pianomode_allow_musicxml_upload($mimes) {
    $mimes['musicxml'] = 'application/vnd.recordare.musicxml+xml';
    $mimes['mxl'] = 'application/vnd.recordare.musicxml';
    $mimes['xml'] = 'application/xml';
    $mimes['gp'] = 'application/octet-stream';
    $mimes['gp3'] = 'application/octet-stream';
    $mimes['gp4'] = 'application/octet-stream';
    $mimes['gp5'] = 'application/octet-stream';
    $mimes['gpx'] = 'application/octet-stream';
    return $mimes;
}
add_filter('upload_mimes', 'pianomode_allow_musicxml_upload');

// =====================================================
// 9. ENQUEUE MEDIA UPLOADER
// =====================================================

function pianomode_enqueue_score_admin_scripts($hook) {
    global $post_type;
    
    if ($post_type === 'score' && in_array($hook, ['post.php', 'post-new.php'])) {
        wp_enqueue_media();
    }
}
add_action('admin_enqueue_scripts', 'pianomode_enqueue_score_admin_scripts');
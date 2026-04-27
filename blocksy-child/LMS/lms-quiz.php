<?php
/**
 * PianoMode LMS Quiz Admin - v3.0
 * Meta boxes for creating/editing quiz challenges within lessons
 */

if (!defined('ABSPATH')) die('Direct access forbidden.');

if (!class_exists('PianoMode_Quiz_Admin')) :

class PianoMode_Quiz_Admin {

    private static $instance = null;

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct() {
        add_action('add_meta_boxes', [$this, 'add_quiz_meta_box']);
        add_action('save_post_pm_lesson', [$this, 'save_quiz_data'], 20, 1);
        add_action('admin_enqueue_scripts', [$this, 'enqueue_admin_scripts']);

        // AJAX for admin
        add_action('wp_ajax_pm_admin_add_challenge', [$this, 'ajax_add_challenge']);
        add_action('wp_ajax_pm_admin_delete_challenge', [$this, 'ajax_delete_challenge']);
        add_action('wp_ajax_pm_admin_add_option', [$this, 'ajax_add_option']);
        add_action('wp_ajax_pm_admin_delete_option', [$this, 'ajax_delete_option']);
    }

    public function enqueue_admin_scripts($hook) {
        if (!in_array($hook, ['post.php', 'post-new.php'])) return;

        global $post;
        if (!$post || $post->post_type !== 'pm_lesson') return;

        wp_enqueue_style(
            'pm-quiz-admin',
            get_stylesheet_directory_uri() . '/LMS/lms-quiz-admin.css',
            [],
            '3.0'
        );
    }

    public function add_quiz_meta_box() {
        add_meta_box(
            'pm_quiz_challenges',
            'Quiz Challenges',
            [$this, 'quiz_meta_box_callback'],
            'pm_lesson',
            'normal',
            'high'
        );
    }

    public function quiz_meta_box_callback($post) {
        wp_nonce_field('pm_quiz_save', 'pm_quiz_nonce');

        global $wpdb;
        $table_challenges = $wpdb->prefix . 'pm_challenges';
        $table_options = $wpdb->prefix . 'pm_challenge_options';

        // Check if table exists
        $table_exists = ($wpdb->get_var("SHOW TABLES LIKE '$table_challenges'") === $table_challenges);

        if (!$table_exists) {
            echo '<p style="color: #d63638; padding: 15px; background: #fcf0f1; border-radius: 8px;">
                Quiz tables not yet created. Visit <strong>Settings > General</strong> and save to trigger table creation, or reload this page.
            </p>';
            return;
        }

        $challenges = $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM $table_challenges WHERE lesson_id = %d ORDER BY sort_order ASC",
            $post->ID
        ));

        ?>
        <div class="pm-quiz-admin-wrapper">

            <div class="pm-quiz-header">
                <h3>Lesson Quiz Questions</h3>
                <p class="description">Add multiple-choice questions for this lesson. Students must answer correctly to progress.</p>
            </div>

            <div id="pm-challenges-list">
                <?php if ($challenges) : ?>
                    <?php foreach ($challenges as $index => $challenge) : ?>
                        <?php
                        $options = $wpdb->get_results($wpdb->prepare(
                            "SELECT * FROM $table_options WHERE challenge_id = %d ORDER BY sort_order ASC",
                            $challenge->id
                        ));
                        $this->render_challenge_row($challenge, $options, $index);
                        ?>
                    <?php endforeach; ?>
                <?php else : ?>
                    <div class="pm-no-challenges">
                        <p>No challenges yet. Click "Add Challenge" to create your first quiz question.</p>
                    </div>
                <?php endif; ?>
            </div>

            <div class="pm-quiz-actions">
                <button type="button" class="button button-primary" id="pm-add-challenge-btn">
                    + Add Challenge
                </button>
            </div>
        </div>

        <script>
        (function() {
            var challengeCount = <?php echo count($challenges); ?>;

            document.getElementById('pm-add-challenge-btn').addEventListener('click', function() {
                var list = document.getElementById('pm-challenges-list');
                var noMsg = list.querySelector('.pm-no-challenges');
                if (noMsg) noMsg.remove();

                var idx = challengeCount++;
                var html = getChallengeHTML(idx);
                list.insertAdjacentHTML('beforeend', html);
            });

            document.addEventListener('click', function(e) {
                // Add option
                if (e.target.classList.contains('pm-add-option-btn')) {
                    var wrapper = e.target.closest('.pm-challenge-row');
                    var optList = wrapper.querySelector('.pm-options-list');
                    var cIdx = wrapper.dataset.index;
                    var oIdx = optList.querySelectorAll('.pm-option-row').length;
                    optList.insertAdjacentHTML('beforeend', getOptionHTML(cIdx, oIdx));
                }

                // Remove challenge
                if (e.target.classList.contains('pm-remove-challenge')) {
                    if (confirm('Remove this challenge and all its options?')) {
                        e.target.closest('.pm-challenge-row').remove();
                    }
                }

                // Remove option
                if (e.target.classList.contains('pm-remove-option')) {
                    e.target.closest('.pm-option-row').remove();
                }
            });

            function getChallengeHTML(idx) {
                return '<div class="pm-challenge-row" data-index="' + idx + '">' +
                    '<div class="pm-challenge-header">' +
                        '<span class="pm-challenge-number">Q' + (idx + 1) + '</span>' +
                        '<button type="button" class="pm-remove-challenge button-link-delete">Remove</button>' +
                    '</div>' +
                    '<div class="pm-challenge-fields">' +
                        '<p><label><strong>Type:</strong></label><br>' +
                            '<select name="pm_challenges[' + idx + '][type]" class="widefat">' +
                                '<option value="select">Multiple Choice (Select)</option>' +
                                '<option value="assist">Assist (Word Bank)</option>' +
                            '</select></p>' +
                        '<p><label><strong>Question:</strong></label><br>' +
                            '<textarea name="pm_challenges[' + idx + '][question]" class="widefat" rows="2" placeholder="Enter the question..."></textarea></p>' +
                        '<p><label><strong>Explanation (shown after answer):</strong></label><br>' +
                            '<textarea name="pm_challenges[' + idx + '][explanation]" class="widefat" rows="2" placeholder="Optional explanation..."></textarea></p>' +
                        '<p><label><strong>Image URL:</strong></label><br>' +
                            '<input type="url" name="pm_challenges[' + idx + '][image_url]" class="widefat" placeholder="Optional image URL"></p>' +
                        '<p><label><strong>Audio URL:</strong></label><br>' +
                            '<input type="url" name="pm_challenges[' + idx + '][audio_url]" class="widefat" placeholder="Optional audio URL"></p>' +
                        '<input type="hidden" name="pm_challenges[' + idx + '][sort_order]" value="' + idx + '">' +
                        '<input type="hidden" name="pm_challenges[' + idx + '][id]" value="">' +
                    '</div>' +
                    '<div class="pm-options-section">' +
                        '<h4>Answer Options</h4>' +
                        '<div class="pm-options-list"></div>' +
                        '<button type="button" class="button pm-add-option-btn">+ Add Option</button>' +
                    '</div>' +
                '</div>';
            }

            function getOptionHTML(cIdx, oIdx) {
                return '<div class="pm-option-row">' +
                    '<div class="pm-option-fields">' +
                        '<input type="text" name="pm_challenges[' + cIdx + '][options][' + oIdx + '][text]" placeholder="Option text..." class="pm-option-text">' +
                        '<label class="pm-correct-label"><input type="radio" name="pm_challenges[' + cIdx + '][correct_option]" value="' + oIdx + '"> Correct</label>' +
                        '<input type="url" name="pm_challenges[' + cIdx + '][options][' + oIdx + '][image_url]" placeholder="Image URL (optional)" class="pm-option-url">' +
                        '<input type="url" name="pm_challenges[' + cIdx + '][options][' + oIdx + '][audio_url]" placeholder="Audio URL (optional)" class="pm-option-url">' +
                        '<input type="hidden" name="pm_challenges[' + cIdx + '][options][' + oIdx + '][id]" value="">' +
                        '<button type="button" class="pm-remove-option button-link-delete">x</button>' +
                    '</div>' +
                '</div>';
            }
        })();
        </script>
        <?php
    }

    private function render_challenge_row($challenge, $options, $index) {
        ?>
        <div class="pm-challenge-row" data-index="<?php echo $index; ?>">
            <div class="pm-challenge-header">
                <span class="pm-challenge-number">Q<?php echo $index + 1; ?></span>
                <button type="button" class="pm-remove-challenge button-link-delete">Remove</button>
            </div>
            <div class="pm-challenge-fields">
                <p>
                    <label><strong>Type:</strong></label><br>
                    <select name="pm_challenges[<?php echo $index; ?>][type]" class="widefat">
                        <option value="select" <?php selected($challenge->type, 'select'); ?>>Multiple Choice (Select)</option>
                        <option value="assist" <?php selected($challenge->type, 'assist'); ?>>Assist (Word Bank)</option>
                    </select>
                </p>
                <p>
                    <label><strong>Question:</strong></label><br>
                    <textarea name="pm_challenges[<?php echo $index; ?>][question]" class="widefat" rows="2"><?php echo esc_textarea($challenge->question); ?></textarea>
                </p>
                <p>
                    <label><strong>Explanation (shown after answer):</strong></label><br>
                    <textarea name="pm_challenges[<?php echo $index; ?>][explanation]" class="widefat" rows="2"><?php echo esc_textarea($challenge->explanation); ?></textarea>
                </p>
                <p>
                    <label><strong>Image URL:</strong></label><br>
                    <input type="url" name="pm_challenges[<?php echo $index; ?>][image_url]" class="widefat" value="<?php echo esc_attr($challenge->image_url); ?>">
                </p>
                <p>
                    <label><strong>Audio URL:</strong></label><br>
                    <input type="url" name="pm_challenges[<?php echo $index; ?>][audio_url]" class="widefat" value="<?php echo esc_attr($challenge->audio_url); ?>">
                </p>
                <input type="hidden" name="pm_challenges[<?php echo $index; ?>][sort_order]" value="<?php echo $index; ?>">
                <input type="hidden" name="pm_challenges[<?php echo $index; ?>][id]" value="<?php echo $challenge->id; ?>">
            </div>
            <div class="pm-options-section">
                <h4>Answer Options</h4>
                <div class="pm-options-list">
                    <?php if ($options) : ?>
                        <?php foreach ($options as $oIdx => $opt) : ?>
                            <div class="pm-option-row">
                                <div class="pm-option-fields">
                                    <input type="text" name="pm_challenges[<?php echo $index; ?>][options][<?php echo $oIdx; ?>][text]" value="<?php echo esc_attr($opt->text); ?>" class="pm-option-text">
                                    <label class="pm-correct-label">
                                        <input type="radio" name="pm_challenges[<?php echo $index; ?>][correct_option]" value="<?php echo $oIdx; ?>" <?php checked($opt->is_correct, 1); ?>> Correct
                                    </label>
                                    <input type="url" name="pm_challenges[<?php echo $index; ?>][options][<?php echo $oIdx; ?>][image_url]" value="<?php echo esc_attr($opt->image_url); ?>" placeholder="Image URL" class="pm-option-url">
                                    <input type="url" name="pm_challenges[<?php echo $index; ?>][options][<?php echo $oIdx; ?>][audio_url]" value="<?php echo esc_attr($opt->audio_url); ?>" placeholder="Audio URL" class="pm-option-url">
                                    <input type="hidden" name="pm_challenges[<?php echo $index; ?>][options][<?php echo $oIdx; ?>][id]" value="<?php echo $opt->id; ?>">
                                    <button type="button" class="pm-remove-option button-link-delete">x</button>
                                </div>
                            </div>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </div>
                <button type="button" class="button pm-add-option-btn">+ Add Option</button>
            </div>
        </div>
        <?php
    }

    public function save_quiz_data($post_id) {
        if (!isset($_POST['pm_quiz_nonce'])) return;
        if (!wp_verify_nonce($_POST['pm_quiz_nonce'], 'pm_quiz_save')) return;
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) return;
        if (!current_user_can('edit_post', $post_id)) return;

        global $wpdb;
        $table_challenges = $wpdb->prefix . 'pm_challenges';
        $table_options = $wpdb->prefix . 'pm_challenge_options';

        // Check tables exist
        if ($wpdb->get_var("SHOW TABLES LIKE '$table_challenges'") !== $table_challenges) return;

        $challenges_data = $_POST['pm_challenges'] ?? [];

        // Get existing challenge IDs for this lesson
        $existing_ids = $wpdb->get_col($wpdb->prepare(
            "SELECT id FROM $table_challenges WHERE lesson_id = %d",
            $post_id
        ));

        $submitted_ids = [];

        foreach ($challenges_data as $idx => $challenge) {
            $challenge_id = !empty($challenge['id']) ? intval($challenge['id']) : 0;
            $type = sanitize_text_field($challenge['type'] ?? 'select');
            $question = sanitize_textarea_field($challenge['question'] ?? '');
            $explanation = sanitize_textarea_field($challenge['explanation'] ?? '');
            $image_url = esc_url_raw($challenge['image_url'] ?? '');
            $audio_url = esc_url_raw($challenge['audio_url'] ?? '');
            $sort_order = intval($challenge['sort_order'] ?? $idx);

            if (empty($question)) continue;

            $data = [
                'lesson_id' => $post_id,
                'type' => $type,
                'question' => $question,
                'explanation' => $explanation,
                'image_url' => $image_url,
                'audio_url' => $audio_url,
                'sort_order' => $sort_order
            ];
            $format = ['%d', '%s', '%s', '%s', '%s', '%s', '%d'];

            if ($challenge_id && in_array($challenge_id, $existing_ids)) {
                // Update
                $wpdb->update($table_challenges, $data, ['id' => $challenge_id], $format, ['%d']);
                $submitted_ids[] = $challenge_id;
            } else {
                // Insert
                $wpdb->insert($table_challenges, $data, $format);
                $challenge_id = $wpdb->insert_id;
                $submitted_ids[] = $challenge_id;
            }

            // Save options
            $correct_option_idx = isset($challenge['correct_option']) ? intval($challenge['correct_option']) : -1;
            $options = $challenge['options'] ?? [];

            // Get existing option IDs
            $existing_opt_ids = $wpdb->get_col($wpdb->prepare(
                "SELECT id FROM $table_options WHERE challenge_id = %d",
                $challenge_id
            ));
            $submitted_opt_ids = [];

            foreach ($options as $oIdx => $opt) {
                $opt_id = !empty($opt['id']) ? intval($opt['id']) : 0;
                $text = sanitize_text_field($opt['text'] ?? '');

                if (empty($text)) continue;

                $opt_data = [
                    'challenge_id' => $challenge_id,
                    'text' => $text,
                    'is_correct' => ($oIdx == $correct_option_idx) ? 1 : 0,
                    'image_url' => esc_url_raw($opt['image_url'] ?? ''),
                    'audio_url' => esc_url_raw($opt['audio_url'] ?? ''),
                    'sort_order' => $oIdx
                ];
                $opt_format = ['%d', '%s', '%d', '%s', '%s', '%d'];

                if ($opt_id && in_array($opt_id, $existing_opt_ids)) {
                    $wpdb->update($table_options, $opt_data, ['id' => $opt_id], $opt_format, ['%d']);
                    $submitted_opt_ids[] = $opt_id;
                } else {
                    $wpdb->insert($table_options, $opt_data, $opt_format);
                    $submitted_opt_ids[] = $wpdb->insert_id;
                }
            }

            // Delete removed options
            $to_delete_opts = array_diff($existing_opt_ids, $submitted_opt_ids);
            foreach ($to_delete_opts as $del_id) {
                $wpdb->delete($table_options, ['id' => $del_id], ['%d']);
            }
        }

        // Delete removed challenges
        $to_delete = array_diff($existing_ids, $submitted_ids);
        foreach ($to_delete as $del_id) {
            $wpdb->delete($table_options, ['challenge_id' => $del_id], ['%d']);
            $wpdb->delete($table_challenges, ['id' => $del_id], ['%d']);
        }

        // Update has_quiz flag
        $has_quiz = !empty($submitted_ids) ? '1' : '0';
        update_post_meta($post_id, '_pm_lesson_has_quiz', $has_quiz);
    }
}

PianoMode_Quiz_Admin::get_instance();

endif;
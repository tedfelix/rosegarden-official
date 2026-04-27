/**
 * Play Page Admin - PianoMode
 * Admin panel interactions: add/remove games, image upload, toggle panels
 */

(function($) {
    'use strict';

    var gameIndex = 0;

    $(document).ready(function() {
        // Set initial index based on existing rows
        gameIndex = $('#pm-games-list .pm-game-row').length;

        // === DRAG & DROP REORDERING ===
        if ($.fn.sortable) {
            $('#pm-games-list').sortable({
                handle: '.pm-game-drag-handle',
                placeholder: 'pm-game-sortable-placeholder',
                axis: 'y',
                opacity: 0.7,
                cursor: 'grabbing',
                update: function() {
                    // Update order numbers in UI
                    $('#pm-games-list .pm-game-row').each(function(i) {
                        $(this).find('.pm-game-order-num').text(i + 1);
                    });
                }
            });
        }

        // Manual numeric order: reorder on blur
        $(document).on('change', '.pm-game-order-input', function() {
            var rows = $('#pm-games-list .pm-game-row').toArray();
            rows.sort(function(a, b) {
                var orderA = parseInt($(a).find('.pm-game-order-input').val()) || 999;
                var orderB = parseInt($(b).find('.pm-game-order-input').val()) || 999;
                return orderA - orderB;
            });
            $('#pm-games-list').empty().append(rows);
            // Update displayed numbers
            $('#pm-games-list .pm-game-row').each(function(i) {
                $(this).find('.pm-game-order-num').text(i + 1);
                $(this).find('.pm-game-order-input').val(i + 1);
            });
        });

        // Toggle game row body
        $(document).on('click', '.pm-game-toggle', function(e) {
            e.preventDefault();
            var body = $(this).closest('.pm-game-row').find('.pm-game-row-body');
            body.slideToggle(200);
            $(this).text(body.is(':visible') ? 'Close' : 'Edit');
        });

        // Remove game row
        $(document).on('click', '.pm-game-remove', function(e) {
            e.preventDefault();
            if (confirm('Remove this game?')) {
                $(this).closest('.pm-game-row').slideUp(200, function() {
                    $(this).remove();
                });
            }
        });

        // Add game
        $('#pm-add-game').on('click', function() {
            var template = $('#pm-game-template').html();
            var html = template.replace(/__INDEX__/g, gameIndex);
            $('#pm-games-list').append(html);
            var newRow = $('#pm-games-list .pm-game-row').last();
            newRow.find('.pm-game-row-body').show();
            gameIndex++;
        });

        // Update row title on input change
        $(document).on('input', '.pm-game-title-input', function() {
            var val = $(this).val() || 'New Game';
            $(this).closest('.pm-game-row').find('.pm-game-row-title').text(val);
        });

        // Image upload
        $(document).on('click', '.pm-upload-image', function(e) {
            e.preventDefault();
            var button = $(this);
            var container = button.closest('.pm-image-field');
            var inputField = container.find('.pm-image-url');
            var preview = container.find('.pm-image-preview');
            var removeBtn = container.find('.pm-remove-image');

            var frame = wp.media({
                title: 'Choose Game Image',
                button: { text: 'Use this image' },
                multiple: false,
                library: { type: 'image' }
            });

            frame.on('select', function() {
                var attachment = frame.state().get('selection').first().toJSON();
                // Use full size for HD quality, fallback to large then original
                var url = attachment.url; // full original
                if (attachment.sizes) {
                    if (attachment.sizes.full) {
                        url = attachment.sizes.full.url;
                    } else if (attachment.sizes.large) {
                        url = attachment.sizes.large.url;
                    }
                }
                inputField.val(url);
                preview.html('<img src="' + url + '" alt="">');
                removeBtn.show();
            });

            frame.open();
        });

        // Remove image
        $(document).on('click', '.pm-remove-image', function(e) {
            e.preventDefault();
            var container = $(this).closest('.pm-image-field');
            container.find('.pm-image-url').val('');
            container.find('.pm-image-preview').html('');
            $(this).hide();
        });
    });

})(jQuery);
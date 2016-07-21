<?php

/**
 * Implements hook_views_plugins().
 */
function libanswers_stats_views_plugins() {
  $plugin = array();
  $plugin['query']['libanswers_stats_plugin_query'] = array(
    'title' => t('Libanswers Stats Query'),
    'help' => t('Libanswers Stats query object.'),
    'handler' => 'libanswers_stats_plugin_query',
  );
  return $plugin;
}


/**
 * Implements hook_views_data().
 *
 * the actual fields from which the user can choose in the view
 */
function libanswers_stats_views_data() {
  $data = array();

  //Base data
  $data['libanswers_stats']['table']['group'] = t('Libanswers Stats');
  $data['libanswers_stats']['table']['base'] = array(
    'title' => t('Libanswers Stats'),
    'help' => t('Query Libanswers Stats'),
    'query class' => 'libanswers_stats_plugin_query'
  );

  //Fields
  $data['libanswers_stats']['title'] = array(
    'title' => t('Type'),
    'help' => t('The type or category of this dataset'),
    'field' => array(
      'handler' => 'libanswers_stats_handler_field',
    ),
  );
  $data['libanswers_stats']['date'] = array(
    'title' => t('Date'),
    'help' => t('The date of this dataset'),
    'field' => array(
      'handler' => 'libanswers_stats_handler_field',
    ),
  );
  $data['libanswers_stats']['count'] = array(
    'title' => t('Count'),
    'help' => t('The total count of this dataset'),
    'field' => array(
      'handler' => 'libanswers_stats_handler_field',
    ),
  );

  return $data;
}
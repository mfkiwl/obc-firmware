/**
 * @ingroup event_manager
 * 
 * @file EventManager_test.c
 * @author Duncan Hamill (dh2g16@soton.ac.uk/duncanrhamill@googlemail.com)
 * @brief Test functionality of the EventManager module.
 * @version 0.1
 * @date 2020-10-29
 * 
 * @copyright Copyright (c) 2020
 * 
 * @defgroup event_manager_test Event Manager Test
 * @{
 */

/* -------------------------------------------------------------------------   
 * INCLUDES
 * ------------------------------------------------------------------------- */

/* Standard library includes */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

/* External library includes */
#include <cmocka.h>

/* Internal includes */
#include "util/debug/Debug_public.h"
#include "system/data_pool/DataPool_public.h"
#include "system/event_manager/EventManager_public.h"

/* -------------------------------------------------------------------------   
 * TESTS
 * ------------------------------------------------------------------------- */

/**
 * @brief Test that the init function behaves as expected
 * 
 * @param state cmocka state
 */
static void EventManager_test_init(void **state) {
    (void) state;

    /* Check that the init function succeeds */
    assert_true(EventManager_init());

    /* Check init flag is set */
    assert_true(DP.EVENTMANAGER.INITIALISED);
    
    /* Check that all pointers in the EventManager static are non-null */
    assert_non_null(EVENTMANAGER.p_raised_events);
    assert_non_null(EVENTMANAGER.p_num_cycles_events_raised);

    /* Check that the list size is the minimum size */
    assert_int_equal(
        DP.EVENTMANAGER.EVENT_LIST_SIZE, 
        EVENTMANAGER_MIN_LIST_SIZE
    );
    
    /* Check there are no events */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 0);
}

/**
 * @brief Test that _raise_event works correctly.
 * 
 * @param state cmocka state.
 */
static void EventManager_test_raise_event(void **state) {
    (void) state;

    /* Raise 64 events, which will require expanding the event list */
    for (int i = 1; i <= 64; i++) {
        assert_true(EventManager_raise_event((Event)i));
    }

    /* Check size and num of events */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 64);
    assert_int_equal(DP.EVENTMANAGER.EVENT_LIST_SIZE, 128);

    /* Check all events are in the list */
    for (int i = 0; i < 64; i++) {
        assert_int_equal(EVENTMANAGER.p_raised_events[i], i + 1);
    }
}

/**
 * @brief Test that _is_event_raised works correctly.
 * 
 * @param state 
 */
static void EventManager_test_is_event_raised(void **state) {
    (void) state;

    /* Check that no event is raised at the start */
    bool is_raised = true;
    assert_true(EventManager_is_event_raised((Event)1, &is_raised));
    assert_false(is_raised);

    /* Raise an event and check it is raised */
    assert_true(EventManager_raise_event((Event)1));
    assert_true(EventManager_is_event_raised((Event)1, &is_raised));
    assert_true(is_raised);
}

/**
 * @brief Test that _poll_event works correctly.
 * 
 * @param state cmocka state
 */
static void EventManager_test_poll_event(void **state) {
    (void) state;

    /* Raise an event */
    assert_true(EventManager_raise_event((Event)2));

    /* Poll that event and check that the raised flag is true */
    bool is_raised = false;
    assert_true(EventManager_poll_event((Event)2, &is_raised));
    assert_true(is_raised);

    /* Check that the event has been removed */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 0);

    /* Poll an event that isn't raised and check the raised flag is false. */
    assert_true(EventManager_poll_event((Event)1, &is_raised));
    assert_false(is_raised);

    /* Check that there are no events */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 0);
}

/**
 * @brief Test that _cleanup_events works correctly.
 * 
 * @param state cmocka state
 */
static void EventManager_test_cleanup_events(void **state) {
    (void) state;

    /* Raise 64 events */
    for (int i = 1; i <= 64; i++) {
        assert_true(EventManager_raise_event((Event)i));
    }

    /* Check all events are present */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 64);

    /* Check all cycle counters are zero */
    for (int i = 0; i < 64; i++) {
        assert_int_equal(EVENTMANAGER.p_num_cycles_events_raised[i], 0);
    }

    /* Call cleanup */
    assert_true(EventManager_cleanup_events());

    /* Check there are still 64 events */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 64);

    /* Check all counters are one */
    for (int i = 0; i < 64; i++) {
        assert_int_equal(EVENTMANAGER.p_num_cycles_events_raised[i], 1);
    }

    /* Call cleanup */
    assert_true(EventManager_cleanup_events());

    /* Check there are no events */
    assert_int_equal(DP.EVENTMANAGER.NUM_RAISED_EVENTS, 0);
}

/**
 * @brief Test that error codes work correctly.
 * 
 * @param state cmocka state
 */
static void EventManager_test_errors(void **state) {
    (void) state;

    /* Init DataPool */
    assert_true(DataPool_init());

    DEBUG_INF("Testing EventManager error codes");

    /* Call a random function which should fail because the EM isn't init */
    assert_false(EventManager_shrink_lists());
    assert_int_equal(
        DP.EVENTMANAGER.ERROR_CODE, 
        EVENTMANAGER_ERROR_NOT_INITIALISED
    );

    /* Now init the EM and test for other errors */
    assert_true(EventManager_init());

    /* Raise more events than the max */
    for (Event evt = (Event)1; evt < EVENTMANAGER_MAX_LIST_SIZE; evt++) {
        assert_true(EventManager_raise_event(evt));
    }
    assert_false(EventManager_raise_event(EVENTMANAGER_MAX_LIST_SIZE + 1));
    assert_int_equal(
        DP.EVENTMANAGER.ERROR_CODE,
        EVENTMANAGER_ERROR_MAX_EVENTS_REACHED
    );

    /* Clear all events */
    assert_true(EventManager_clear_all_events());
}

/**
 * @brief Setup function for EventManager tests, which inits EM.
 * 
 * @param state cmocka state
 */
static int EventManager_test_setup(void **state) {
    (void) state;

    /* Init DataPool */
    assert_true(DataPool_init());
    
    if (!EventManager_init()) {
        return -1;
    }

    return 0;
}

/**
 * @brief Teardown function for EventManager tests, which just destroys the EM.
 * 
 * @param state cmocka state
 */
static int EventManager_test_teardown(void **state) {
    (void) state;
    
    if (DP.EVENTMANAGER.ERROR_CODE != EVENTMANAGER_ERROR_NONE) {
        DEBUG_ERR(
            "DP.EVENTMANAGER.ERROR_CODE = %d", 
            DP.EVENTMANAGER.ERROR_CODE
        );
    }

    EventManager_destroy();

    return 0;
}

/* -------------------------------------------------------------------------   
 * TEST GROUP
 * ------------------------------------------------------------------------- */

/**
 * @brief Tests to run for the event manager. 
 */
const struct CMUnitTest eventmanager_tests[] = {
    /* No setup for the init test, which calls init itself */
    cmocka_unit_test_teardown(
        EventManager_test_init, 
        EventManager_test_teardown
    ),
    cmocka_unit_test_setup_teardown(
        EventManager_test_raise_event,
        EventManager_test_setup,
        EventManager_test_teardown
    ),
    cmocka_unit_test_setup_teardown(
        EventManager_test_is_event_raised,
        EventManager_test_setup,
        EventManager_test_teardown
    ),
    cmocka_unit_test_setup_teardown(
        EventManager_test_poll_event,
        EventManager_test_setup,
        EventManager_test_teardown
    ),
    cmocka_unit_test_setup_teardown(
        EventManager_test_cleanup_events,
        EventManager_test_setup,
        EventManager_test_teardown
    ),
    cmocka_unit_test_teardown(
        EventManager_test_errors,
        EventManager_test_teardown
    )
};

/** @} */ /* End of event_manager_test */
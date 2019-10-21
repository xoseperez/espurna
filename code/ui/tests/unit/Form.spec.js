import {mount} from '@vue/test-utils'
import Form from '../../src/components/Form'
import Input from '../../src/components/Input'


describe('Form', () => {
    let cmp;
    beforeEach(() => {
        cmp = mount(Form, {
            slots: {
                default: {
                    render: (h) => {
                        return h('div',
                            [
                                h(Input, {props: {name: 'test1', type: "text"}}),
                                h(Input, {props: {name: 'test2', type: "select", options: ['0', '1', '2', '3']}}),
                                h(Input, {props: {name: 'test3', type: "switch"}})
                            ]
                        )
                    }
                }
            }
        });
    });
    it('renders the inputs', () => {
        const form = cmp.find(Form);
        expect(form.findAll(Input).length).toBe(3)
    });

    it('updates its values', () => {
        const form = cmp.find(Form);
        const inputs = form.findAll(Input);

        const in1 = inputs.at(0).find('input');
        in1.element.value = "Hello world";
        in1.trigger('input');

        const in2 = inputs.at(1).find('select');
        in2.element.value = 1;
        in2.trigger('change');

        const in3 = inputs.at(2).find('label');
        in3.trigger('click');

        expect(form.vm.values).toMatchObject({test1: "Hello world", test2: 1, test3: true});

        in3.trigger('click');

        expect(form.vm.values).toMatchObject({test1: "Hello world", test2: 1, test3: false});
    })
});